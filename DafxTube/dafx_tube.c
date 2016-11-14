
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
#define TUBE_URI "www.linux.ime.usp.br/~vitorsamora/mac0499/dafx-tube.lv2"

/**
   In the code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability. They need to match the definitions in the
   *.ttl file.
*/
typedef enum {
	INPUT = 0,
	OUTPUT = 1, 
   GAIN = 2,
   Q = 3,
   DIST = 4,
   RH = 5,
   RL = 6,
   MIX = 7
} PortIndex;

/**
   Define a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method, being passed back through the 'instance' parameter.
**/
typedef struct {
   const float* input;    // lv2 audio port;
   float*       output;   // lv2 audio port;
   const float* gain;     // lv2 control port
   const float* q;     // lv2 control port
   const float* dist;     // lv2 control port
   const float* rh;     // lv2 control port
   const float* rl;     // lv2 control port
   const float* mix;     // lv2 control port
   float* lastX;
   float* lastX2;
   float* lastY;
   float* lastY2;
   float* lastY_LP;
} Tube;

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
	Tube* tube = (Tube*)instance;

	if (tube == NULL ) {
		return;
	}

	switch ((PortIndex)port) {
   case INPUT:
      tube->input = (const float*)data;
      break;
   case OUTPUT:
      tube->output = (float*)data;
      break;
   case GAIN:
      tube->gain = (const float*)data;
      break;
   case Q:
      tube->q = (const float*)data;
      break;
   case DIST:
      tube->dist = (const float*)data;
      break;
   case RH:
      tube->rh = (const float*)data;
      break;
   case RL:
      tube->rl = (const float*)data;
      break;
   case MIX:
      tube->mix = (const float*)data;
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
	uint32_t pos;
	const Tube* tube = (Tube*)instance;
	const float* const input  = tube->input;
	float* const       output = tube->output;
   float gain = *(tube->gain);
   float q = *(tube->q);
   float dist = *(tube->dist);
   float rh = *(tube->rh);
   float rl = *(tube->rl);
   float mix = *(tube->mix);
   float extra[5000];
   float extra2[5000];

   float max = 0, maxZ = 0, maxY = 0, y;

	if (tube == NULL) {
		fprintf(stderr, "DAFX_TUBE: run() called with NULL instance parameter.\n");
		return;
	}

	for (pos = 0; pos < n_samples; pos++) {
      if (fabs(input[pos]) > max)
         max = fabs(input[pos]);
   }

   for (pos = 0; pos < n_samples; pos++) {
      if (max > 0) extra[pos] = input[pos]*gain/max; /*Nomalização (y no livro)*/
      else extra[pos] = 0;
   }

   if (q == 0) {
      for (pos = 0; pos < n_samples; pos++) {
         extra2[pos] = extra[pos]/(1 - exp(-dist*extra[pos])); /*z no livro*/
      }
      for (pos = 0; pos < n_samples; pos++) {
         if (extra[pos] == q)
            extra2[pos] = 1/dist;
      }
   }
   else {
      for (pos = 0; pos < n_samples; pos++) {
         extra2[pos] = (extra[pos] - q)/(1 - exp(-dist*(extra[pos]-q))) + q/(1 - exp(dist*q));
      }
      for (pos = 0; pos < n_samples; pos++) {
         if (extra[pos] == q)
            extra2[pos] = 1/dist + Q/(1-exp(dist*q));
      }
   }

   //MAXZ
   for (pos = 0; pos < n_samples; pos++) {
      if (fabs(extra2[pos]) > maxZ)
         maxZ = fabs(extra2[pos]);
   }

   for (pos = 0; pos < n_samples; pos++) {
      if (max > 0 && maxZ > 0) y = mix*extra2[pos]*max/maxZ + (1-mix)*input[pos];
      else y = 0;
      extra[pos] = y;
   }

   //MAXY
   for (pos = 0; pos < n_samples; pos++) {
      if (fabs(extra[pos]) > maxY)
         maxY = fabs(extra[pos]);
   }

   for (pos = 0; pos < n_samples; pos++) {
      if (max > 0 && maxY > 0) extra[pos] = extra[pos]*max/maxY;
      else extra[pos] = 0;
   }

   /* FILTERING:
      HP: y(n) = x(n) - 2*x(n-1) + x(n-2) + 2*rh*y(n-1) - rh*rh*y(n-2)
      LP: y(n) = (1-rl)*x(n) + rl*y(n-1) 
   */
   //HP:
   for (pos = 0; pos < n_samples; pos++) {
      y = extra[pos] - 2*(*(tube->lastX)) + *(tube->lastX2) + 2*rh*(*(tube->lastY))- rh*rh*(*(tube->lastY2));
      *(tube->lastX2) = *(tube->lastX);
      *(tube->lastX)= extra[pos];
      *(tube->lastY2) = *(tube->lastY);
      *(tube->lastY)= y;
      extra[pos] = y;
   }
   //LP:
   for (pos = 0; pos < n_samples; pos++) {
      y = (1-rl)*extra[pos] + rl*(*(tube->lastY_LP));
      *(tube->lastY_LP) = y;
      extra[pos] = y;
   }

   //OUT:
   for (pos = 0; pos < n_samples; pos++) {
      output[pos] = extra[pos];
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
	Tube* tube = (Tube*)malloc(sizeof(Tube));

   tube->lastX = malloc(sizeof(float));
   tube->lastX2 = malloc(sizeof(float));
   tube->lastY = malloc(sizeof(float));
   tube->lastY2 = malloc(sizeof(float));
   tube->lastY_LP = malloc(sizeof(float));

   *(tube->lastX) = 0;
   *(tube->lastX2) = 0;
   *(tube->lastY) = 0;
   *(tube->lastY2) = 0;
   *(tube->lastY_LP) = 0;

	return (LV2_Handle)tube;
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
   Tube* tube = (Tube*) instance;
   *(tube->lastX) = 0;
   *(tube->lastX2) = 0;
   *(tube->lastY) = 0;
   *(tube->lastY2) = 0;
   *(tube->lastY_LP) = 0;
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
	TUBE_URI,
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
