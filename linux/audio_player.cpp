//
// Created by boris on 29.07.23.
//
#include "audio_player.h"
#include <iostream>
#include <map>
#include "audio_player_exception.h"

#define GST_PLAY_FLAG_DOWNLOAD (1 << 7)

AudioPlayer::AudioPlayer(FlEventChannel* eventChannel)
{
    _eventSender = std::make_unique<FlutterEventSender>(eventChannel);

    gst_init(nullptr, nullptr);

    _playbin = gst_element_factory_make("playbin", "playbin");
    guint flags;
    g_object_get (_playbin, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_DOWNLOAD;
    g_object_set (_playbin, "flags", flags, NULL);

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
    if(!_isUrlSet) return;

    GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        gst_object_unref (_playbin);
        _playbin = nullptr;
        throw AudioPlayerException("Unable to set the pipeline to GST_STATE_PLAYING");
    }
    else if (ret == GST_STATE_CHANGE_NO_PREROLL)
    {
        _isLive = true;
    }
}

void AudioPlayer::pause()
{
    GstState state;
    gst_element_get_state(_playbin, &state, nullptr, GST_CLOCK_TIME_NONE);
    if(state != GST_STATE_PLAYING) return;

    GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        throw AudioPlayerException("Unable to set the pipeline to GST_STATE_PAUSED");
    }
}

void AudioPlayer::setVolume(double value)
{
    if(!_playbin) return;

    if (value > 1)
    {
        value = 1;
    }
    else if (value < 0)
    {
        value = 0;
    }
    g_object_set(G_OBJECT(_playbin), "volume", value, nullptr);
}

void AudioPlayer::setUrl(const char* urlString)
{
    _isUrlSet = false;
    gst_element_set_state(_playbin, GST_STATE_NULL);
    g_object_set(GST_OBJECT(_playbin), "uri", urlString, nullptr);

    if (_playbin->current_state == GST_STATE_READY) return;

    GstStateChangeReturn ret = gst_element_set_state(_playbin, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        throw AudioPlayerException("Unable to set the pipeline to GST_STATE_READY");
    }

    _isBuffered = false;
    _isUrlSet = true;
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

//            std::cout << "State changed from " << states[old_state] << " to " << states[new_state] << std::endl;

            data->_eventSender->send("audio.playingState", states[new_state]);

            if(new_state == GST_STATE_PLAYING || new_state == GST_STATE_PAUSED)
            {
                GstQuery* query = gst_query_new_seeking(GST_FORMAT_TIME);
                if (gst_element_query(data->_playbin, query))
                {
                    gst_query_parse_seeking(query, nullptr, &data->_seekEnabled, nullptr, nullptr);
                }
                else
                {
                    g_printerr("Seeking query failed.\n");
                }
                gst_query_unref(query);
            }

            break;
        }
        case GST_MESSAGE_EOS:
        {
            std::cout << "Playback ended" << std::endl;
            data->_eventSender->send("audio.completed", true);

            break;
        }
        case GST_MESSAGE_DURATION_CHANGED:
        {
            const gint64 duration = data->duration();
            std::cout << "[Duration update]: " << duration << std::endl;

            data->_eventSender->send("audio.duration", duration / 1'000'000);

            break;
        }
        case GST_MESSAGE_BUFFERING:
        {
            data->_isBuffered = true;
            /* If the stream is live, we do not care about buffering. */
            if (data->_isLive) break;

            gst_message_parse_buffering(message, &data->_bufferingLevel);
            g_print("Buffering (%3d%%)\r", data->_bufferingLevel);

            /* Wait until buffering is complete before start/resume playing */
//            if (data->_bufferingLevel < 100)
//                gst_element_set_state (data->_playbin, GST_STATE_PAUSED);
//            else
//                gst_element_set_state (data->_playbin, GST_STATE_PLAYING);
//            break;
        }
        case GST_MESSAGE_CLOCK_LOST:
            /* Get a new clock */
            gst_element_set_state (data->_playbin, GST_STATE_PAUSED);
            gst_element_set_state (data->_playbin, GST_STATE_PLAYING);
            break;
        case GST_MESSAGE_ELEMENT:
        {
            g_message("Message element: %s", gst_structure_get_name(gst_message_get_structure(message)));
            break;
        }
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
    return duration;
}

gboolean AudioPlayer::_onRefreshTick(AudioPlayer* data)
{
    if(!data->_playbin) return false;

    GstState playbinState;
    gst_element_get_state(data->_playbin, &playbinState, nullptr, GST_CLOCK_TIME_NONE);
    if (playbinState == GST_STATE_PLAYING) {
        if (!gst_element_query_position(data->_playbin, GST_FORMAT_TIME, &data->_position)) {
            std::cerr << "Could not query current position." << std::endl;
            return 0;
        }

        data->_eventSender->send("audio.position", data->_position);
    }

    if(data->_isBuffered && data->_downloadProgress < 1'000'000) // less than 100%
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
            g_message("Downloading progress: %f%%\n", percent * 100.0);

            data->_eventSender->send("audio.buffering", percent);
        }
    }

    return true;
}

/// Changes position and speed of the playback
/// \param position in nanoseconds
/// \param rate 1.0 - normal speed, 2.0 double speed, 0.5 - half speed,
/// positive values are forward, negative ones are backward
void AudioPlayer::_seek(gint64 position, gdouble rate)
{
    if(!_playbin || !_seekEnabled) return;

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

    gboolean result = gst_element_seek(_playbin,
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

///
/// \param position in milliseconds
void AudioPlayer::seek(gint64 position)
{
    gint64 newPosition = position * GST_MSECOND;
    if(newPosition == _position) return;

    _seek(newPosition, _rate);
}

void AudioPlayer::setRate(double rate)
{
    if(rate == _rate) return;

    _seek(_position, rate);
}
