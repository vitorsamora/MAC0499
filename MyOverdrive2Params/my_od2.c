
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
#define OD2_URI "www.linux.ime.usp.br/~vitorsamora/mac0499/my-overdrive2.lv2"
/*#define TH ((float)1)/((float)3)*/

/**
   In the code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability. They need to match the definitions in the
   *.ttl file.
*/
typedef enum {
	INPUT   = 0,
	OUTPUT  = 1, 
   TH   = 2,
   TH2   = 3
} PortIndex;

/**
   Define a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method, being passed back through the 'instance' parameter.
**/
typedef struct {
   const float* input;    // lv2 audio port;
   float*       output;   // lv2 audio port;
   const float* th;     // lv2 control port
   const float* th2;     // lv2 control port
} Od2;

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
	Od2* od2 = (Od2*)instance;

	if (od2 == NULL ) {
		return;
	}

	switch ((PortIndex)port) {
   case INPUT:
      od2->input = (const float*)data;
      break;
   case OUTPUT:
      od2->output = (float*)data;
      break;
   case TH:
      od2->th = (const float*)data;
      break;
   case TH2:
      od2->th2 = (const float*)data;
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
   float th, th2, percentage;
   float a, b, c, d, e;
	const Od2* od2 = (Od2*)instance;
	const float* const input  = od2->input;
	float* const       output = od2->output;

	// check that we have a valid instance before
	// checking for valid buffers
	if (od2 == NULL) {
		fprintf(stderr, "MY_OD2: run() called with NULL instance parameter.\n");
		return;
	}

   th = 1.0 - *(od2->th);
   /*if (th == 0.0) th = nextafterf(th, 1.0);
   if (th == 1.0) th = nextafterf(th, 0.0);*/

   percentage = 1.0 - *(od2->th2);
   th2 = th + percentage*(1.0 - th);
   if (th2 == (1.0 - sqrt(1.0 - pow(th, 2))))
      th2 = nextafterf(th2, nextafterf(1.0, 0.0)); //nextafterf(1.0, 0.0) returns the largest representable number less than 1.0;

   b = 0;

   e = (pow(th, 2) - 2*b*(th2 - pow(th2, 2)/2)) / (pow(th, 2) - 2*(th2 - pow(th2, 2)/2));
   d = 2*(b - e)/pow(th, 2);
   c = -d/2;
   a = d + 2*th*c;

	for (pos = 0; pos < n_samples; pos++) {
		if (fabs(input[pos]) > th2) {
			if (input[pos] > 0) {
				output[pos] = 1;
         }
			if (input[pos] < 0) {
				output[pos] = -1;
         }
      }
      else if (fabs(input[pos]) < th) {
         output[pos] = a*input[pos];
      }
      else /*th <= fabs(input[pos]) <= th2*/ {
         if (input[pos] > 0) {
            output[pos] = e + (d * input[pos]) + (c * pow(input[pos], 2));
         }
         if (input[pos] < 0) {
            output[pos] = (-1 * (e - (d * input[pos]) + (c * pow(input[pos], 2))));
         }
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
	Od2* od2 = (Od2*)malloc(sizeof(Od2));

	return (LV2_Handle)od2;
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
	OD2_URI,
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
