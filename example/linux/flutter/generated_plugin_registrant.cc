//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <audio_player_gst/audio_player_gst_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) audio_player_gst_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "AudioPlayerGstPlugin");
  audio_player_gst_plugin_register_with_registrar(audio_player_gst_registrar);
}
