
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
#define WAH_URI "www.linux.ime.usp.br/~vitorsamora/mac0499/dafx-wah.lv2"

#define PI 3.14159265358979323846

/**
   In the code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability. They need to match the definitions in the
   *.ttl file.
*/
typedef enum {
	INPUT = 0,
	OUTPUT = 1, 
   CONTROL = 2,
   BANDWIDTH = 3,
   MIX = 4,
} PortIndex;

/**
   Define a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method, being passed back through the 'instance' parameter.
**/
typedef struct {
   const float* input;    // lv2 audio port;
   float*       output;   // lv2 audio port;
   const float* control;     // lv2 control port
   const float* bandwidth;     // lv2 control port
   const float* mix;     // lv2 control port
   float* lastX;
   float* lastX2;
   float* lastY;
   float* lastY2;
   float* lastY_LP;
   int* fs;
} Wah;

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
	Wah* wah = (Wah*)instance;

	if (wah == NULL ) {
		return;
	}

	switch ((PortIndex)port) {
   case INPUT:
      wah->input = (const float*)data;
      break;
   case OUTPUT:
      wah->output = (float*)data;
      break;
   case CONTROL:
      wah->control = (const float*)data;
      break;
   case BANDWIDTH:
      wah->bandwidth = (const float*)data;
      break;
   case MIX:
      wah->mix = (const float*)data;
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
	const Wah* wah = (Wah*)instance;
	const float* const input  = wah->input;
	float* const       output = wah->output;
   int fs = *(wah->fs);
   float fb = *(wah->bandwidth) * fs;
   float fc = *(wah->control) * fs;
   float mix = *(wah->mix);
   float c, d, y, y1;

   d = -cos(2*PI*fc/fs);

   c = (tan(PI*fb/fs) - 1)/(tan(2*PI*fb/fs) + 1);


   /* FILTERING:
      AP: A(z) = (-c + d*(1-c)*z^-1 + z^-2)/(1 + d*(1-c)*z^-1 - c*z^-2) 
         y(n) = -c * x(n) + d * (1 - c) * x(n-1) + x(n-2) - d * (1 - c) * y(n - 1) + c * y(n - 2)
      BP: H(z) = 0.5 * (1 - A(z))
      Wah: (1-mix)*X(z) + mix*H(z)
   */

   for (pos = 0; pos < n_samples; pos++) {
      // y = 0.5*((1 - c) * input[pos] + 2*d*(1-c)*(*(wah->lastX)) + (1 - c)*(*(wah->lastX2))) - d*(1-c)*(*(wah->lastY)) + c*(*(wah->lastY2));
      y1 = -c*input[pos] + d*(1 - c)*(*(wah->lastX)) + (*(wah->lastX2)) -d*(1 - c)*(*(wah->lastY)) + c*(*(wah->lastY2));
      y = 0.5 * (input[pos] - y1);
      *(wah->lastX2) = *(wah->lastX);
      *(wah->lastX)= input[pos];
      *(wah->lastY2) = *(wah->lastY);
      *(wah->lastY)= y1;
      output[pos] = input[pos]*(1.0 - mix) + y*mix;
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
	Wah* wah = (Wah*)malloc(sizeof(Wah));

   wah->lastX = malloc(sizeof(float));
   wah->lastX2 = malloc(sizeof(float));
   wah->lastY = malloc(sizeof(float));
   wah->lastY2 = malloc(sizeof(float));
   wah->lastY_LP = malloc(sizeof(float));
   wah->fs = malloc(sizeof(int));

   *(wah->lastX) = 0;
   *(wah->lastX2) = 0;
   *(wah->lastY) = 0;
   *(wah->lastY2) = 0;
   *(wah->lastY_LP) = 0;
   *(wah->fs) = rate;

	return (LV2_Handle)wah;
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
   Wah* wah = (Wah*) instance;
   *(wah->lastX) = 0;
   *(wah->lastX2) = 0;
   *(wah->lastY) = 0;
   *(wah->lastY2) = 0;
   *(wah->lastY_LP) = 0;
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
	WAH_URI,
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
