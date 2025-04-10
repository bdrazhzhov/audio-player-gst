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
        gst_app_src_set_size(source, size);
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

    uint64_t dataSize = unusedSize;
    const uint64_t availableSize = self->downloader.decryptOffset() - self->offset;
    if(availableSize < dataSize) dataSize = availableSize;

    GstBuffer* gstBuffer = gst_buffer_new_allocate(nullptr, dataSize, nullptr);
    gst_buffer_fill(gstBuffer, 0, self->downloader.data() + self->offset, dataSize);

    self->offset += dataSize;

    GstFlowReturn ret;
    g_signal_emit_by_name(self->source, "push-buffer", gstBuffer, &ret);
    gst_buffer_unref(gstBuffer);
}

void AppSrc::appSourceSeek(GstElement*, const guint64 offset, AppSrc* self)
{
    std::cout << "appSourceSeek: " << offset << std::endl;

    self->offset = offset;
}
