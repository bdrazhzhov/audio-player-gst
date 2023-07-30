//
// Created by boris on 29.07.23.
//
#include "audio_player.h"
#include <iostream>
#include <map>

#define GST_PLAY_FLAG_DOWNLOAD (1 << 7)

AudioPlayer::AudioPlayer(FlEventChannel* eventChannel) : _eventChannel(eventChannel)
{
    gst_init(nullptr, nullptr);

//    _pipeline = gst_parse_launch(R"(playbin uri=file:///media/Media/Music/Avril\ Lavigne/Let\ Go/03.\ Sk8ter\ Boi.mp3)", nullptr);
//    _pipeline = gst_parse_launch(R"(playbin uri=https://drive.google.com/uc?id=1LfoEzQ55EgQZHzxm-C6Z50eThKmdrHDl&export=download)", nullptr);
    _playbin = gst_element_factory_make("playbin", "playbin");
    guint flags;
    g_object_get (_playbin, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_DOWNLOAD;
    g_object_set (_playbin, "flags", flags, NULL);
//    g_object_set (_playbin, "ring-buffer-max-size", (guint64)20'000'000, NULL);
//    g_object_set (_playbin, "buffer-size", (guint64)20'000'000, NULL);

    _bus = gst_element_get_bus(_playbin);
    gst_bus_add_watch(_bus, (GstBusFunc)AudioPlayer::_onBusMessage, this);
    _refreshTimer = g_timeout_add(100, (GSourceFunc)AudioPlayer::_onRefreshTick, this);
}

AudioPlayer::~AudioPlayer()
{
    g_source_remove(_refreshTimer);
    gst_object_unref(_bus);
    gst_element_set_state(_playbin, GST_STATE_NULL);
    gst_object_unref(_playbin);
}

void AudioPlayer::play()
{
    GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (_playbin);
        _playbin = nullptr;
    } else if (ret == GST_STATE_CHANGE_NO_PREROLL) {
        _isLive = true;
    }
}

void AudioPlayer::pause()
{
    gst_element_set_state(_playbin, GST_STATE_PAUSED);
}

void AudioPlayer::setVolume(double value)
{
    if(!_playbin) return;

    if (value > 1) {
        value = 1;
    } else if (value < 0) {
        value = 0;
    }
    g_object_set(G_OBJECT(_playbin), "volume", value, nullptr);
}

void AudioPlayer::setUrl(const char* urlString)
{
    std::cout << "setUrl: " << urlString << std::endl;

    gst_element_set_state(_playbin, GST_STATE_NULL);
    g_object_set(GST_OBJECT(_playbin), "uri", urlString, nullptr);
    if (_playbin->current_state != GST_STATE_READY) {
        GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_READY);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            throw "Unable to set the pipeline to GST_STATE_READY.";
        }
    }
}

gboolean AudioPlayer::_onBusMessage(GstBus* /*bus*/, GstMessage* message, AudioPlayer* data)
{
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;

            gst_message_parse_error(message, &err, &debug);
            std::cerr << "[GError]: " << err->code << " - " << err->message << std::endl;
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
            static std::map<GstState, std::string> states = {
                {GST_STATE_VOID_PENDING, "GST_STATE_VOID_PENDING"},
                {GST_STATE_NULL,         "GST_STATE_NULL"},
                {GST_STATE_READY,        "GST_STATE_READY"},
                {GST_STATE_PAUSED,       "GST_STATE_PAUSED"},
                {GST_STATE_PLAYING,      "GST_STATE_PLAYING"}
            };
            GstState old_state, new_state;
            gst_message_parse_state_changed(message, &old_state, &new_state, nullptr);

            if(data->_eventChannel)
            {
                g_autoptr(FlValue) map = fl_value_new_map();
                fl_value_set_string(map, "event", fl_value_new_string("audio.playingState"));
                fl_value_set_string(map, "value", fl_value_new_string(states[new_state].c_str()));
                fl_event_channel_send(data->_eventChannel, map, nullptr, nullptr);
            }

            if(new_state == GST_STATE_PLAYING || new_state == GST_STATE_PAUSED)
            {
                GstQuery* query = gst_query_new_seeking(GST_FORMAT_TIME);
                if (gst_element_query(data->_playbin, query))
                {
                    gst_query_parse_seeking(query, nullptr, &data->_seekEnabled, nullptr, nullptr);
                }
                else
                {
                    g_printerr("Seeking query failed.");
                }
                gst_query_unref(query);
            }

            break;
        }
        case GST_MESSAGE_EOS:
//            data->OnPlaybackEnded();
            std::cout << "Playback ended" << std::endl;
            break;
        case GST_MESSAGE_DURATION_CHANGED:
        {
            const gint64 duration = data->duration();
            std::cout << "[Duration update]: " << duration << std::endl;
            if(data->_eventChannel)
            {
                g_autoptr(FlValue) map = fl_value_new_map();
                fl_value_set_string(map, "event", fl_value_new_string("audio.duration"));
                fl_value_set_string(map, "value", fl_value_new_int(duration));
                fl_event_channel_send(data->_eventChannel, map, nullptr, nullptr);
            }
            break;
        }
//        case GST_MESSAGE_BUFFERING:
//        {
//            /* If the stream is live, we do not care about buffering. */
//            if (data->_isLive) break;
//
//            gst_message_parse_buffering (message, &data->_bufferingLevel);
//            g_print ("Buffering (%3d%%)\r", data->_bufferingLevel);
//
//            /* Wait until buffering is complete before start/resume playing */
//            if (data->_bufferingLevel < 100)
//                gst_element_set_state (data->_playbin, GST_STATE_PAUSED);
//            else
//                gst_element_set_state (data->_playbin, GST_STATE_PLAYING);
//            break;
//        }
        case GST_MESSAGE_CLOCK_LOST:
            /* Get a new clock */
            gst_element_set_state (data->_playbin, GST_STATE_PAUSED);
            gst_element_set_state (data->_playbin, GST_STATE_PLAYING);
            break;
        default:
            // For more GstMessage types see:
            // https://gstreamer.freedesktop.org/documentation/gstreamer/gstmessage.html?gi-language=c#enumerations
            static std::map<GstMessageType,std::string> messageTypes = {
                {GST_MESSAGE_UNKNOWN , "GST_MESSAGE_UNKNOWN"},
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
                {GST_MESSAGE_CLOCK_LOST, "GST_MESSAGE_CLOCK_LOST"},
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
            std::cout << "Unprocessed message type: " << messageTypes[GST_MESSAGE_TYPE(message)] << std::endl;
            break;
    }

    // Continue watching for messages
    return TRUE;
}

gint64 AudioPlayer::duration()
{
    gint64 duration = 0;
    if (!gst_element_query_duration(_playbin, GST_FORMAT_TIME, &duration)) {
        std::cerr << "Could not query current duration." << std::endl;
        return 0;
    }
    return duration / 1000000;
}

gboolean AudioPlayer::_onRefreshTick(AudioPlayer* data)
{
    if(!data->_playbin) return false;

    GstState playbinState;
    gst_element_get_state(data->_playbin, &playbinState, nullptr, GST_CLOCK_TIME_NONE);
    if (playbinState == GST_STATE_PLAYING) {
        gint64 position = 0;
        if (!gst_element_query_position(data->_playbin, GST_FORMAT_TIME, &position)) {
            std::cerr << "Could not query current position." << std::endl;
            return 0;
        }

        g_autoptr(FlValue) map = fl_value_new_map();
        fl_value_set_string(map, "event", fl_value_new_string("audio.position"));
        fl_value_set_string(map, "value", fl_value_new_int(position));
        fl_event_channel_send(data->_eventChannel, map, nullptr, nullptr);
    }

    if(data->_downloadProgress < 1'000'000) // less than 100%
    {
        GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);
        gboolean result = gst_element_query(data->_playbin, query);
        if(result)
        {
            // using first range (with index 0)
            // looks like it contains all downloading progress
            // when GST_PLAY_FLAG_DOWNLOAD is set
            gint64 start;
            gst_query_parse_nth_buffering_range (query, 0, &start, &data->_downloadProgress);
            double percent = static_cast<double>(data->_downloadProgress) / 1'000'000.0;
            std::cout << "Downloading progress: " << percent * 100.0 << "%" << std::endl;

            if(data->_eventChannel)
            {
                g_autoptr(FlValue) map = fl_value_new_map();
                fl_value_set_string(map, "event", fl_value_new_string("audio.buffering"));
                fl_value_set_string(map, "value", fl_value_new_float(percent));
                fl_event_channel_send(data->_eventChannel, map, nullptr, nullptr);
            }
        }
    }

    return true;
}

void AudioPlayer::seek(gint64 position)
{
    if(!_playbin || !_seekEnabled) return;

    gst_element_seek_simple(_playbin, GST_FORMAT_TIME,
                            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), position * GST_MSECOND);
}
