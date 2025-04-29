//
// Created by boris on 29.07.23.
//

#include "audio_player.h"

#include <iostream>
#include <map>
#include <gst/app/gstappsrc.h>
#include "audio_player_exception.h"

#define GST_PLAY_FLAG_DOWNLOAD (1 << 7)
#define g_print(format, ...)

AudioPlayer::AudioPlayer(FlEventChannel* eventChannel) : _sendEvent(eventChannel)
{
    gst_init(nullptr, nullptr);

    _playbin = gst_element_factory_make("playbin", "playbin");
    _scaleTempo = gst_element_factory_make("scaletempo", "scaletempo");

    if(!_playbin || !_scaleTempo)
    {
        std::cerr << "Could not create playbin or scaletempo elements" << std::endl;
        return;
    }

    g_object_set(GST_OBJECT(_playbin), "audio-filter", _scaleTempo, NULL);
    g_signal_connect(_playbin, "source-setup", G_CALLBACK(sourceSetup), this);
    g_object_set(_playbin, "uri", "appsrc://", NULL);

    _bus = gst_element_get_bus(_playbin);
    gst_bus_add_watch(_bus, reinterpret_cast<GstBusFunc>(_onBusMessage), this);

    _refreshTimer = g_timeout_add(100, reinterpret_cast<GSourceFunc>(_onRefreshTick), this);
    g_signal_connect(_playbin, "notify::volume", G_CALLBACK(AudioPlayer::_onVolumeChanged), this);
}

AudioPlayer::~AudioPlayer()
{
    g_source_remove(_refreshTimer);
    gst_object_unref(_bus);
    gst_element_set_state(_playbin, GST_STATE_NULL);
    gst_object_unref(_scaleTempo);
    gst_object_unref(_playbin);
}

void AudioPlayer::play()
{
    if(!_isUrlSet) return;

    const GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_PLAYING);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        gst_object_unref(_playbin);
        _playbin = nullptr;
        throw AudioPlayerException("Unable to set the pipeline to GST_STATE_PLAYING");
    }
}

void AudioPlayer::pause() const
{
    GstState state;
    gst_element_get_state(_playbin, &state, nullptr, GST_CLOCK_TIME_NONE);
    if(state != GST_STATE_PLAYING) return;

    const GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_PAUSED);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        throw AudioPlayerException("Unable to set the pipeline to GST_STATE_PAUSED");
    }
}

void AudioPlayer::setVolume(double value)
{
    if(!_playbin) return;

    if(value > 1)
    {
        value = 1;
    }
    else if(value < 0)
    {
        value = 0;
    }

    _volume = value;

    _isVolumeAboutToSet = true;
    g_object_set(GST_OBJECT(_playbin), "volume", _volume, nullptr);
    _isVolumeAboutToSet = false;
}

void AudioPlayer::setUrl(const char* urlString, const char* encryptionKey)
{
    _isUrlSet = false;
    _isDownloaded = false;

//    std::cout << "[audio_player_gst]: setUrl: " << urlString << std::endl;

    GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_NULL);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        throw AudioPlayerException("setUrl: Unable to set the pipeline to GST_STATE_NULL");
    }

    _appSrc.init(urlString, encryptionKey);
    _position = 0;

    if(_playbin->current_state == GST_STATE_READY) return;

    ret = gst_element_set_state(_playbin, GST_STATE_READY);
    if(ret == GST_STATE_CHANGE_FAILURE)
    {
        throw AudioPlayerException("setUrl: Unable to set the pipeline to GST_STATE_READY");
    }

    _isUrlSet = true;
}

void AudioPlayer::seek(const gint64 position)
{
    const gint64 newPosition = position * GST_MSECOND;
    if(newPosition == _position) return;

    _position = newPosition;

    _seek(newPosition, _rate);
}

void AudioPlayer::setRate(const double rate)
{
    if(rate == _rate) return;
    _rate = rate;

    _seek(_position, _rate);
}

gint64 AudioPlayer::duration() const
{
    gint64 duration = 0;
    if(!gst_element_query_duration(_playbin, GST_FORMAT_TIME, &duration))
    {
        return 0;
    }

    return duration;
}

gboolean AudioPlayer::_onBusMessage(GstBus*, GstMessage* message, AudioPlayer* self)
{
    switch(GST_MESSAGE_TYPE(message))
    {
    case GST_MESSAGE_ERROR:
    {
        GError* err;
        gchar* debug;

        gst_message_parse_error(message, &err, &debug);

        g_printerr("[audio_player_gst]: Error received from element '%s': %s\n",
                   GST_OBJECT_NAME(message->src), err->message);
        g_printerr("[audio_player_gst]: Debugging information: %s\n", debug ? debug : "none");
        //g_print("[audio_player_gst][GError]: %d, %s", err->code, err->message);

        g_error_free(err);
        g_free(debug);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
        if(GST_MESSAGE_SRC(message) != GST_OBJECT(self->_playbin)) break;

        static std::map<GstState, std::string> states = {
            {GST_STATE_VOID_PENDING, "GST_STATE_VOID_PENDING"},
            {GST_STATE_NULL, "GST_STATE_NULL"},
            {GST_STATE_READY, "GST_STATE_READY"},
            {GST_STATE_PAUSED, "GST_STATE_PAUSED"},
            {GST_STATE_PLAYING, "GST_STATE_PLAYING"}
        };
        GstState old_state, new_state;
        gst_message_parse_state_changed(message, &old_state, &new_state, nullptr);

//        std::cout << "[audio_player_gst]: State changed from " << states[old_state]
//            << " to " << states[new_state] << std::endl;

        if(old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED)
        {
            break;
        }
        else if(new_state == GST_STATE_READY)
        {
            GstStateChangeReturn ret =
                gst_element_set_state(self->_playbin, GST_STATE_PAUSED);
            if (ret == GST_STATE_CHANGE_FAILURE)
            {
                g_print("Unable to set the pipeline from GST_STATE_READY to "
                        "GST_STATE_PAUSED.");
            }
        }
        else if (new_state == GST_STATE_PLAYING && old_state == GST_STATE_PAUSED)
        {
//            std::cout << "[audio_player_gst]: Requesting seek" << std::endl;
            GstQuery *query = gst_query_new_seeking(GST_FORMAT_TIME);

            if(gst_element_query(self->_playbin, query))
            {
                gst_query_parse_seeking(query, nullptr, &self->_seekEnabled, nullptr, nullptr);
            }
            else
            {
                //g_printerr("[audio_player_gst]: Seeking query failed.\n");
//                std::cout << "[audio_player_gst]: Seeking query failed." << std::endl;
            }

            gst_query_unref(query);
        }

            self->_sendEvent("audio.playingState", states[new_state]);

        break;
    }
    case GST_MESSAGE_EOS:
    {
        // std::cout << "[audio_player_gst]: Playback finished" << std::endl;
        gst_element_set_state(self->_playbin, GST_STATE_NULL);
        self->_sendEvent("audio.completed", true);

        break;
    }
    case GST_MESSAGE_DURATION_CHANGED:
    {
        const gint64 duration = self->duration();
//        g_print("[Duration update]: %ld\n", duration);

        self->_sendEvent("audio.duration", duration / 1'000'000);

        break;
    }
    case GST_MESSAGE_ELEMENT:
    {
        //g_message("[audio_player_gst]: Message element: %s", gst_structure_get_name(gst_message_get_structure(message)));
        break;
    }
    default:
        // For more GstMessage types see:
        // https://gstreamer.freedesktop.org/documentation/gstreamer/gstmessage.html?gi-language=c#enumerations
        static std::map<GstMessageType, std::string> messageTypes = {
            {GST_MESSAGE_UNKNOWN, "GST_MESSAGE_UNKNOWN"},
            {GST_MESSAGE_EOS, "GST_MESSAGE_EOS"},
            {GST_MESSAGE_ERROR, "GST_MESSAGE_ERROR"},
            {GST_MESSAGE_WARNING, "GST_MESSAGE_WARNING"},
            {GST_MESSAGE_INFO, "GST_MESSAGE_INFO"},
            {GST_MESSAGE_TAG, "GST_MESSAGE_TAG"},
            {GST_MESSAGE_BUFFERING, "GST_MESSAGE_BUFFERING"},
            {GST_MESSAGE_STATE_CHANGED, "GST_MESSAGE_STATE_CHANGED"},
            {GST_MESSAGE_STATE_DIRTY, "GST_MESSAGE_STATE_DIRTY"},
            {GST_MESSAGE_STEP_DONE, "GST_MESSAGE_STEP_DONE"},
            {GST_MESSAGE_CLOCK_PROVIDE, "GST_MESSAGE_CLOCK_PROVIDE"},
            //                {GST_MESSAGE_CLOCK_LOST, "GST_MESSAGE_CLOCK_LOST"},
            {GST_MESSAGE_NEW_CLOCK, "GST_MESSAGE_NEW_CLOCK"},
            {GST_MESSAGE_STRUCTURE_CHANGE, "GST_MESSAGE_STRUCTURE_CHANGE"},
            {GST_MESSAGE_STREAM_STATUS, "GST_MESSAGE_STREAM_STATUS"},
            {GST_MESSAGE_APPLICATION, "GST_MESSAGE_APPLICATION"},
            {GST_MESSAGE_ELEMENT, "GST_MESSAGE_ELEMENT"},
            {GST_MESSAGE_SEGMENT_START, "GST_MESSAGE_SEGMENT_START"},
            {GST_MESSAGE_SEGMENT_DONE, "GST_MESSAGE_SEGMENT_DONE"},
            {GST_MESSAGE_DURATION_CHANGED, "GST_MESSAGE_DURATION_CHANGED"},
            {GST_MESSAGE_LATENCY, "GST_MESSAGE_LATENCY"},
            {GST_MESSAGE_ASYNC_START, "GST_MESSAGE_ASYNC_START"},
            {GST_MESSAGE_ASYNC_DONE, "GST_MESSAGE_ASYNC_DONE"},
            {GST_MESSAGE_REQUEST_STATE, "GST_MESSAGE_REQUEST_STATE"},
            {GST_MESSAGE_STEP_START, "GST_MESSAGE_STEP_START"},
            {GST_MESSAGE_QOS, "GST_MESSAGE_QOS"},
            {GST_MESSAGE_PROGRESS, "GST_MESSAGE_PROGRESS"},
            {GST_MESSAGE_TOC, "GST_MESSAGE_TOC"},
            {GST_MESSAGE_RESET_TIME, "GST_MESSAGE_RESET_TIME"},
            {GST_MESSAGE_STREAM_START, "GST_MESSAGE_STREAM_START"},
            {GST_MESSAGE_NEED_CONTEXT, "GST_MESSAGE_NEED_CONTEXT"},
            {GST_MESSAGE_HAVE_CONTEXT, "GST_MESSAGE_HAVE_CONTEXT"},
            {GST_MESSAGE_EXTENDED, "GST_MESSAGE_EXTENDED"},
            {GST_MESSAGE_DEVICE_ADDED, "GST_MESSAGE_DEVICE_ADDED"},
            {GST_MESSAGE_DEVICE_REMOVED, "GST_MESSAGE_DEVICE_REMOVED"},
            {GST_MESSAGE_PROPERTY_NOTIFY, "GST_MESSAGE_PROPERTY_NOTIFY"},
            {GST_MESSAGE_STREAM_COLLECTION, "GST_MESSAGE_STREAM_COLLECTION"},
            {GST_MESSAGE_STREAMS_SELECTED, "GST_MESSAGE_STREAMS_SELECTED"},
            {GST_MESSAGE_REDIRECT, "GST_MESSAGE_REDIRECT"},
            {GST_MESSAGE_DEVICE_CHANGED, "GST_MESSAGE_DEVICE_CHANGED"},
            {GST_MESSAGE_INSTANT_RATE_REQUEST, "GST_MESSAGE_INSTANT_RATE_REQUEST"},
        };
//        std::cout << "[audio_player_gst]: Unprocessed message type: ";
//        std::cout << messageTypes[GST_MESSAGE_TYPE(message)] << std::endl;
        break;
    }

    // Continue watching for messages
    return true;
}

gboolean AudioPlayer::_onRefreshTick(AudioPlayer* self)
{
    if(!self->_playbin) return false;

    GstState playbinState;
    gst_element_get_state(self->_playbin, &playbinState, nullptr, GST_CLOCK_TIME_NONE);
    if(playbinState == GST_STATE_PLAYING)
    {
        if(!gst_element_query_position(self->_playbin, GST_FORMAT_TIME, &self->_position))
        {
//            std::cout << "[audio_player_gst]: Could not query current position." << std::endl;
            return 0;
        }

        self->_sendEvent("audio.position", self->_position);
    }

    if(!self->_isDownloaded)
    {
        const float progress = self->_appSrc.downloadProgress();
        if(progress == 100.0)
        {
            self->_isDownloaded = true;
        }

        self->_sendEvent("audio.buffering", progress);
        // std::cerr << "Download progress: " << progress << std::endl;
    }

    // std::cout << "Position: " << self->_position << ", Download progress: " << self->_downloadProgress << std::endl;

    return true;
}

void AudioPlayer::_onVolumeChanged(GstElement* volumeElement, GParamSpec*, AudioPlayer* self)
{
    if(self->_isVolumeAboutToSet)
    {
        self->_isVolumeAboutToSet = false;
        return;
    }

    g_object_get(volumeElement, "volume", &self->_volume, NULL);
    self->_sendEvent("audio.volume", self->_volume);
//    std::cout << "Volume changed: " << self->_volume << std::endl;
}

void AudioPlayer::_seek(const gint64 position, const gdouble rate)
{
    if(!_seekEnabled) return;

    gint64 start, end;
    if(_rate > 0)
    {
        start = position;
        end = duration();
    }
    else
    {
        start = 0;
        end = position;
    }

    const gboolean result = gst_element_seek(_playbin,
                                             rate,
                                             GST_FORMAT_TIME,
                                             static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                             GST_SEEK_TYPE_SET,
                                             start,
                                             GST_SEEK_TYPE_SET,
                                             end);

    if(result)
    {
        _rate = rate;
    }
    else
    {
        throw AudioPlayerException("Unable to set seek position or playing rate");
    }
}

void AudioPlayer::sourceSetup(GstElement*, GstElement* source, AudioPlayer* self)
{
    const std::string name = G_OBJECT_TYPE_NAME(source);
    if(name != "GstAppSrc") return;

    self->_appSrc.sourceSetup(source);
}
