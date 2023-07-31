//
// Created by boris on 29.07.23.
//
#pragma once
#include <gst/gst.h>
#include <flutter_linux/flutter_linux.h>

class AudioPlayer
{
    FlEventChannel* _eventChannel;
    GstElement* _playbin = nullptr;
    GstBus* _bus = nullptr;
    guint _refreshTimer{};
    bool _isBuffered = false;
    gint _bufferingLevel{};
    bool _isLive = false;
    gint64 _downloadProgress{};
    gboolean _seekEnabled = false;
    gint64 _position{};
    gdouble _rate = 1.0;

    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, AudioPlayer *data);
    static gboolean _onRefreshTick(AudioPlayer *data);
    void _seek(gint64 position, gdouble rate);

public:
    explicit AudioPlayer(FlEventChannel* eventChannel);
    ~AudioPlayer();

    void play();
    void pause();
    void setVolume(double value);
    void setUrl(const char* urlString);
    void seek(gint64 position);
    void setRate(double rate);
    gint64 duration();
};
