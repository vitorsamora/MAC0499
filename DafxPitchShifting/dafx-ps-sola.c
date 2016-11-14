
/** Include standard C headers */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/**
   LV2 headers are based on the URI of the specification they come from, so a
   consistent convention can be used even for unofficial extensions.  The URI
   of the core LV2 specification is <http://lv2plug.in/ns/lv2core>, by
   replacing `http:/` with `lv2` any header in the specification bundle can be
   included, in this case `lv2.h`.
*/
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

/**
   The URI is the identifier for a plugin, and how the host associates this
   implementation in code with its description in data. If this URI does not
   match that used in the data files, the host will fail to load the plugin.
*/
#define PS_URI "www.linux.ime.usp.br/~vitorsamora/mac0499/dafx-ps-sola.lv2"

#define PI 3.14159265358979323846
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/**
   In the code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability. They need to match the definitions in the
   *.ttl file.
*/
typedef enum {
	INPUT   = 0,
	OUTPUT  = 1, 
   ALPHA   = 2
} PortIndex;

/**
   Define a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method, being passed back through the 'instance' parameter.
**/
typedef struct {
	// Port buffers
   const float* input;    // lv2 audio port;
   float*       output;   // lv2 audio port;
   const float* alpha;    // lv2 control port
   float* last_input;
   float* last_L_out;
   int* is_first;
   int* last_n;
   int* last_L;
} Ps;


void copy(float *source, int ini, float *destination, int len) {
   int i;

   for (i = 0; i < len; i++) 
      destination[i] = source[ini + i];
}


int maxSimIndex(float *u, int uLen, float *v, int vLen) {
   int i, j, index;
   float min, sum, term;

   for (i = -uLen; i < vLen; i++) {
      for (j = i, sum = 0; j < i + uLen; j++) {
         if (j < 0) { 
            term = fabs(u[j - i] - 0);
         }
         else if (j <= vLen) { 
            term = fabs(u[j - i] - v[j]);
         }
         else {
            term = fabs(u[j - i] - 0);
         }

         if (term == 0) sum += -1;
         else sum += term;
      }
      if (i == -uLen || sum < min) {
         min = sum;
         index = i;
      }
   }

   return index;
}


int maxSimIndex2(float *u, int uLen, float *v, int vLen) {
   int i, j, index;
   float min, sum;

   for (i = 0; i < vLen; i++) {
      for (j = i, sum = 0; j < i + uLen; j++) {
         if (j < vLen) sum += fabs(u[j - i] - v[j]);
         else sum += fabs(u[j - i] - 0);
      }
      if (i == 0 || sum < min) {
         min = sum;
         index = i;
      }
   }

   return index;
}


// /**
//    The `connect_port()` method is called by the host to connect a particular
//    port to a buffer.  The plugin must store the data location, but data may not
//    be accessed except in run().

//    This method is in the ``audio'' threading class, and is called in the same
//    context as run().
// */
static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Ps* ps = (Ps*)instance;

	if (ps == NULL ) {
		return;
	}

	switch ((PortIndex)port) {
   case INPUT:
      ps->input = (const float*)data;
      break;
   case OUTPUT:
      ps->output = (float*)data;
      break;
   case ALPHA:
      ps->alpha = (const float*)data;
      break;
	}
}

/**
   The `run()` method is the main process function of the plugin.  It processes
   a block of audio in the audio context.  Since this plugin is
   `lv2:hardRTCapable`, `run()` must be real-time safe, so blocking (e.g. with
   a mutex) or memory allocation are not allowed.
*/
static void
run(LV2_Handle instance, uint32_t n_samples)
{
   
   int lfen, lx;
   int data_size;
   float x[2048];

   float alpha, M, term;
   int Sa = 2048/16, N = 2048/4, Ss, i, j, maxIndex, fadeLen, resLen, L, cut;
	const Ps* ps = (Ps*)instance;
	const float* const input  = ps->input;
	float* const       output = ps->output;
   float grain[2048], overlap[256], grainL[256], result[2*4096], tail[4096], in[2*4096];
	
	if (ps == NULL) {
		fprintf(stderr, "DAFX_PS: run() called with NULL instance parameter.\n");
		return;
	}

   if (*(ps->is_first)) {
      for (i = 0; i < n_samples; i++) {
         ps->last_input[i] = input[i];
      }
      for (i = 0; i < 256; i++) {
         ps->last_L_out[i] = 0.0;
      }
      *(ps->is_first) = 0;
      *(ps->last_n) = n_samples;
      *(ps->last_L) = round(Sa * alpha / (2.0));

      for (i = 0; i < n_samples; i++) {
         output[i] = 0.0;
      }
   }

   else {

      data_size = n_samples + *(ps->last_n);
      alpha = *(ps->alpha);
      M = ceilf(data_size / (float)Sa);
      Ss = round(Sa * alpha);
      L = round(Sa * alpha / (2.0));
      resLen = N;

      for (i = 0; i < *(ps->last_n); i++) {
         in[i] = ps->last_input[i];
      }

      for (i = 0; i < n_samples; i++) {
         in[i + *(ps->last_n)] = input[i];
      }


   /*% **** Main TimeScaleSOLA loop*****/
      copy(in, 0, result, data_size);

      for (i = 1; i < M; i++) {
         copy(in, i*Sa, grain, N);

         copy(grain, 0, grainL, L);
         copy(result, i*Ss, overlap, L);
         maxIndex = maxSimIndex(grainL, L, overlap, L);
         

         cut = i*Ss + maxIndex;
         copy(result, cut, tail, resLen - cut);
         
         for (j = cut; j < resLen; j++) {
            term = (j - cut)/((float)(resLen - cut));
            tail[j - cut] *= 1.0 - term;
            tail[j - cut] += grain[j - cut] * term;
         }
         
         fadeLen = resLen - cut;
         
         for (j = 0; j < fadeLen; j++) {
            result[cut + j] = tail[j];
         }
         for (j = 0; j < N - fadeLen; j++) {
            result[cut + fadeLen + j] = grain[fadeLen + j];
         }
         resLen = cut + N;
      }
   /*% **** end TimeScaleSOLA loop*****/

      lfen = data_size;

      lx = floor(lfen * alpha);

      for (i = 0; i < lfen; i++) {
         x[i] = i*lx/((float)lfen);
      }

      for (i = 0; i < lfen; i++) {
         term = (x[i] - floor(x[i]));
         tail[i] = result[(int)floor(x[i])] * (1.0 - term);
         tail[i] += result[(int)floor(x[i]) + 1] * term;
      }

      copy(ps->last_L_out, 0, overlap, *(ps->last_L));
      maxIndex = maxSimIndex2(overlap, *(ps->last_L), tail, N - L);

      for (i = 0; i < *(ps->last_L); i++) {
         term = (i)/((float)(*(ps->last_L) - 1));
         tail[i + maxIndex] *= term;
         tail[i + maxIndex] += ps->last_L_out[i] * (1.0 - term);
      }

      for (i = 0; i < n_samples; i++) {
         ps->last_input[i] = in[i + *(ps->last_n)];
      }

      for (i = 0; i < L; i++)
         ps->last_L_out[i] = tail[n_samples + maxIndex + i];

      *(ps->last_n) = n_samples;
      *(ps->last_L) = L;
       
      for (i = 0; i < n_samples; i++) {
         output[i] = tail[i + maxIndex];
      }

   }

}

/**
   The `instantiate()` function is called by the host to create a new plugin
   instance.  The host passes the plugin descriptor, sample rate, and bundle
   path for plugins that need to load additional resources (e.g. waveforms).
   The features parameter contains host-provided features defined in LV2
   extensions, but this simple plugin does not use any.

   This function is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features
	    )
{

	Ps* ps = (Ps*)malloc(sizeof(Ps));

   ps->last_L_out = malloc(256 * sizeof(float));
   ps->last_input = malloc(4096 * sizeof(float));
   ps->is_first = malloc(sizeof(int));
   ps->last_n = malloc(sizeof(int));
   ps->last_L = malloc(sizeof(int));

   *(ps->is_first) = 1;
   *(ps->last_n) = 0;
   *(ps->last_L) = 0;

	return (LV2_Handle)ps;
}

/**
   The `activate()` method is called by the host to initialise and prepare the
   plugin instance for running.  The plugin must reset all internal state
   except for buffer locations set by `connect_port()`.  Since this plugin has
   no other internal state, this method does nothing.

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
activate(LV2_Handle instance)
{
}

/**
   The `deactivate()` method is the counterpart to `activate()`, and is called by
   the host after running the plugin.  It indicates that the host will not call
   `run()` again until another call to `activate()` and is mainly useful for more
   advanced plugins with ``live'' characteristics such as those with auxiliary
   processing threads.  As with `activate()`, this plugin has no use for this
   information so this method does nothing.

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
deactivate(LV2_Handle instance)
{
}


/**
   Destroy a plugin instance (counterpart to `instantiate()`).

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

/**
   The `extension_data()` function returns any extension data supported by the
   plugin.  Note that this is not an instance method, but a function on the
   plugin descriptor.  It is usually used by plugins to implement additional
   interfaces.  This plugin does not have any extension data, so this function
   returns NULL.

   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
static const void*
extension_data(const char* uri)
{
	return NULL;
}

/**
   Every plugin must define an `LV2_Descriptor`.  It is best to define
   descriptors statically to avoid leaking memory and non-portable shared
   library constructors and destructors to clean up properly.
*/
static const LV2_Descriptor descriptor = {
	PS_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

/**
   The `lv2_descriptor()` function is the entry point to the plugin library.  The
   host will load the library and call this function repeatedly with increasing
   indices to find all the plugins defined in the library.  The index is not an
   indentifier, the URI of the returned descriptor is used to determine the
   identify of the plugin.

   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
