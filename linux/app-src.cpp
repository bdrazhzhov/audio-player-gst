// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#include "app-src.h"

#include <iostream>

AppSrc::AppSrc()
{
    downloader.gotSizeCallback([this](const int64_t size)
    {
        if(source) gst_app_src_set_size(source, size);
    });
}

void AppSrc::init(const char* url, const char* key)
{
    downloader.start(url, key);
    offset = 0;
}

void AppSrc::sourceSetup(GstElement* source)
{
    this->source = GST_APP_SRC(source);
    g_object_set(G_OBJECT(source),
                 "format", GST_FORMAT_BYTES,
                 "is-live", FALSE,
                 "block", TRUE,
                 "stream-type", GST_APP_STREAM_TYPE_SEEKABLE,
                 NULL);
    g_signal_connect(source, "need-data", G_CALLBACK(appSourcePush), this);
    g_signal_connect(source, "seek-data", G_CALLBACK(appSourceSeek), this);
}

void AppSrc::appSourcePush(GstElement*, const guint unusedSize, AppSrc* self)
{
    // std::cout << "appSourcePush: " << self->offset << " / " << self->downloader.dataSize() << std::endl;

    // Wait briefly for downloader to catch up. Needed for seek/probing reads.
    constexpr guint maxWaitMs = 3000;
    guint waitMs = 0;
    while(self->offset >= self->downloader.decryptOffset() && self->downloader.running() && waitMs < maxWaitMs)
    {
        g_usleep(10 * 1000);
        waitMs += 10;
    }

    const uint64_t availableEnd = self->downloader.decryptOffset();
    const uint64_t totalSize = self->downloader.dataSize();

    if(self->offset >= availableEnd)
    {
        if(!self->downloader.running() && self->offset >= totalSize)
        {
            GstFlowReturn ret;
            g_signal_emit_by_name(self->source, "end-of-stream", &ret);
        }
        return;
    }

    uint64_t dataSize = unusedSize;
    const uint64_t availableSize = availableEnd - self->offset;
    if(availableSize < dataSize) dataSize = availableSize;

    if(dataSize == 0) return;

    GstBuffer* gstBuffer = gst_buffer_new_allocate(nullptr, dataSize, nullptr);
    gst_buffer_fill(gstBuffer, 0, self->downloader.data() + self->offset, dataSize);

    self->offset += dataSize;

    GstFlowReturn ret;
    g_signal_emit_by_name(self->source, "push-buffer", gstBuffer, &ret);
    gst_buffer_unref(gstBuffer);
}

gboolean AppSrc::appSourceSeek(GstElement*, const guint64 offset, AppSrc* self)
{
//    std::cout << "appSourceSeek: " << offset << std::endl;

    const uint64_t totalSize = self->downloader.dataSize();
    self->offset = offset > totalSize ? totalSize : offset;

    return true;
}
