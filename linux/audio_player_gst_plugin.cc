#include "include/audio_player_gst/audio_player_gst_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>

#include "audio_player_gst_plugin_private.h"
#include "audio_player.h"
#include "audio_player_exception.h"

#define AUDIO_PLAYER_GST_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), audio_player_gst_plugin_get_type(), \
                              AudioPlayerGstPlugin))

#define printf(format, ...)

struct _AudioPlayerGstPlugin
{
    GObject parent_instance;
};

AudioPlayer* player;

G_DEFINE_TYPE(AudioPlayerGstPlugin, audio_player_gst_plugin, g_object_get_type())

// Called when a method call is received from Flutter.
static void audio_player_gst_plugin_handle_method_call(
        AudioPlayerGstPlugin* self,
        FlMethodCall* method_call)
{
    g_autoptr(FlMethodResponse) response;

    const gchar* method = fl_method_call_get_name(method_call);
    FlValue* args = fl_method_call_get_args(method_call);

    printf("[audio_player_gst]: Method name: %s\n", method);

    try {
        if(strcmp(method, "setUrl") == 0)
        {
            const gchar* url = fl_value_get_string(args);
            printf("%s\n", url);
            player->setUrl(url);
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
        }
        else if(strcmp(method, "play") == 0)
        {
            player->play();
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
        }
        else if(strcmp(method, "pause") == 0)
        {
            player->pause();
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
        }
        else if(strcmp(method, "setVolume") == 0)
        {
            double volume = args == nullptr ? 1.0 : fl_value_get_float(args);
            printf("%f\n", volume);
            player->setVolume(volume);
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
        }
        else if(strcmp(method, "seek") == 0)
        {
            if(args)
            {
                gint64 position = fl_value_get_int(args);
                printf("%ld\n", position);
                player->seek(position);
            }

            response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
        }
        else if(strcmp(method, "setRate") == 0)
        {
            double rate = args == nullptr ? 1.0 : fl_value_get_float(args);
            player->setRate(rate);
            printf("%f\n", rate);
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
        }
        else
        {
            response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
        }
    }
    catch (const AudioPlayerException& exception) {
        response = FL_METHOD_RESPONSE(fl_method_error_response_new("audio_player_error", exception.what(), nullptr));
        printf("[audio_player_gst]: Exception!\n");
    }

    fl_method_call_respond(method_call, response, nullptr);
}

static void audio_player_gst_plugin_dispose(GObject* object)
{
    G_OBJECT_CLASS(audio_player_gst_plugin_parent_class)->dispose(object);
}

static void audio_player_gst_plugin_class_init(AudioPlayerGstPluginClass* klass)
{
    delete player;
    G_OBJECT_CLASS(klass)->dispose = audio_player_gst_plugin_dispose;
}

static FlBinaryMessenger *binaryMessenger;

static void audio_player_gst_plugin_init(AudioPlayerGstPlugin* self)
{
    g_autoptr(FlStandardMethodCodec) eventCodec = fl_standard_method_codec_new();
    auto eventChannel = fl_event_channel_new(binaryMessenger,
                                             "audio_player_gst/events",
                                             FL_METHOD_CODEC(eventCodec));
    player = new AudioPlayer(eventChannel);
}

static void method_call_cb(FlMethodChannel* /*channel*/, FlMethodCall* method_call,
                           gpointer user_data)
{
    AudioPlayerGstPlugin* plugin = AUDIO_PLAYER_GST_PLUGIN(user_data);
    audio_player_gst_plugin_handle_method_call(plugin, method_call);
}

void audio_player_gst_plugin_register_with_registrar(FlPluginRegistrar* registrar)
{
    binaryMessenger = fl_plugin_registrar_get_messenger(registrar);
    AudioPlayerGstPlugin* plugin = AUDIO_PLAYER_GST_PLUGIN(
            g_object_new(audio_player_gst_plugin_get_type(), nullptr));


    g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
    g_autoptr(FlMethodChannel) channel = fl_method_channel_new(binaryMessenger, "audio_player_gst", FL_METHOD_CODEC(codec));
    fl_method_channel_set_method_call_handler(channel, method_call_cb, g_object_ref(plugin), g_object_unref);

    g_object_unref(plugin);
}
