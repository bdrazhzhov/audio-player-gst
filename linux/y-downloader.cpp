// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#include "y-downloader.h"

#include <cinttypes>
#include <iostream>
#include <mutex>
#include <libsoup/soup.h>

#define CHUNK_SIZE 4096

std::mutex startMutex;

YDownloader::~YDownloader() {
    {
        std::lock_guard lk(startMutex);
        isRunning = false;
    }
    if (worker && worker->joinable()) worker->join();
}

void YDownloader::start(const char* url, const char* key)
{
    std::lock_guard lock(startMutex);

    if(worker && worker->joinable())
    {
        isRunning = false;
        worker->join();
    }

    dataOffset = 0;
    needDecryption = !!key;
    if (needDecryption) ydec.init(key);
    _decryptOffset = 0;

    isRunning = true;
    std::string urlString(url);
    worker = std::make_unique<std::thread>([urlString, this] {
        processDownload(urlString.c_str());
    });
}

bool YDownloader::establishConnection(const char* url, ConnectionData& connection) const
{
    constexpr int maxConnAttempts = 3;
    constexpr int retryDelaySeconds = 2;
    int connAttempts = 0;
    GError* error = nullptr;

    while(connAttempts < maxConnAttempts)
    {
        if(connection.session)
        {
            g_object_unref(connection.session);
            connection.session = nullptr;
        }
        if(connection.msg)
        {
            g_object_unref(connection.msg);
            connection.msg = nullptr;
        }
        connection.session = soup_session_new();
        connection.msg = soup_message_new("GET", url);

        if(dataOffset > 0)
        {
            char range[64];
            snprintf(range, sizeof(range), "bytes=%lu-", dataOffset);
            soup_message_headers_append(soup_message_get_request_headers(connection.msg), "Range", range);
        }

//        throw std::runtime_error(std::string("Download failed ") + std::to_string(std::string(url).size()));
        connection.stream = soup_session_send(connection.session, connection.msg, nullptr, &error);
        if(error || !connection.stream)
        {
			connAttempts++;
            std::cerr << "Connection error (retry " << connAttempts
                << "): " << (error ? error->message : "unknown error") << std::endl;
            g_clear_error(&error);
            std::this_thread::sleep_for(std::chrono::seconds(retryDelaySeconds));
        }
        else
        {
            return true;
        }
    }

    return false;
}

bool YDownloader::readDataChunk(const char* url, ConnectionData& connection)
{
    constexpr int maxReadAttempts = 3;
    constexpr int retryDelaySeconds = 2;
    int readAttempts = 0;
    GError* error = nullptr;

    while(readAttempts < maxReadAttempts)
    {
        const gssize readBytes = g_input_stream_read(connection.stream, buffer.data() + dataOffset, CHUNK_SIZE, nullptr, &error);
        if(error)
        {
			readAttempts++;
            std::cerr << "Read error (retry " << readAttempts << "): " << error->message << std::endl;
            g_clear_error(&error);

            if(readAttempts < maxReadAttempts)
            {
                if(connection.stream)
                {
                    g_object_unref(connection.stream);
                    connection.stream = nullptr;
                }
                if(connection.msg)
                {
                    g_object_unref(connection.msg);
                    connection.msg = nullptr;
                }
                if(connection.session)
                {
                    g_object_unref(connection.session);
                    connection.session = nullptr;
                }
                std::this_thread::sleep_for(std::chrono::seconds(retryDelaySeconds));
                establishConnection(url, connection);
            }
            else
            {
                return false;
            }
        }
        else
        {
            if(readBytes <= 0)
            {
                decrypt();
                return false;
            }

            dataOffset += readBytes;
            decrypt();
            return true;
        }
    }
    return false;
}

void YDownloader::processDownload(const char* url)
{
    ConnectionData connection{};

    if(!establishConnection(url, connection))
    {
        std::cerr << "Could not establish connection to server" << std::endl;
        isRunning = false;
        return;
    }

    if(dataOffset == 0)
    {
        if(SOUP_STATUS_IS_SUCCESSFUL(soup_message_get_status(connection.msg)))
        {
            SoupMessageHeaders* response_headers = soup_message_get_response_headers(connection.msg);
            const goffset contentLength = soup_message_headers_get_content_length(response_headers);
            buffer.resize(contentLength);
            if(sizeCallback)
                sizeCallback(contentLength);
        }
        else
        {
            std::cerr << "Server error: " << soup_message_get_status(connection.msg) << std::endl;
            if(connection.stream) g_object_unref(connection.stream);
            if(connection.msg) g_object_unref(connection.msg);
            if(connection.session) g_object_unref(connection.session);
            isRunning = false;
            return;
        }
    }

    while(isRunning && dataOffset < buffer.size())
    {
        if(!readDataChunk(url, connection)) break;
    }

    if(connection.stream) g_object_unref(connection.stream);
    if(connection.msg) g_object_unref(connection.msg);
    if(connection.session) g_object_unref(connection.session);
    isRunning = false;
}

void YDownloader::decrypt()
{
    if(!needDecryption)
    {
        _decryptOffset = dataOffset;
        return;
    }

    uint64_t dataSize = dataOffset - _decryptOffset;
    if(dataOffset < buffer.size())
    {
        const uint8_t sizeCorrection = dataSize % 16;
        if(sizeCorrection > 0) dataSize -= sizeCorrection;
    }

    ydec.decrypt(buffer.data() + _decryptOffset, dataSize);
    _decryptOffset += dataSize;
}

void YDownloader::cancel()
{
    isRunning = false;
}

uint8_t YDownloader::progress() const
{
    if(_decryptOffset == 0) return 0.f;
    if(_decryptOffset == buffer.size()) return 100.f;

    return 100 * _decryptOffset / buffer.size();
}
//============================================
YDownloader2::YDownloader2()
{
    session = soup_session_new();
    soupLogger = soup_logger_new(SOUP_LOGGER_LOG_HEADERS);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(soupLogger));
}

YDownloader2::~YDownloader2()
{
    g_clear_object(&session);
    g_clear_object(&cancellable);
    g_clear_object(&loop);
}

void YDownloader2::start(const char* url, const char* key)
{
    std::lock_guard lock(startMutex);
    dataOffset = 0;
    needDecryption = !!key;
    if (needDecryption) yDec.init(key);
    _decryptOffset = 0;

    g_main_loop_unref(loop);
    loop = g_main_loop_new(nullptr, FALSE);

    g_clear_object(&cancellable);
    cancellable = g_cancellable_new();

    message = soup_message_new(SOUP_METHOD_GET, url);
    g_signal_connect(message, "got-headers", G_CALLBACK(onHeaders), this);
    soup_session_send_async(session, message, G_PRIORITY_DEFAULT, cancellable, onResponse, this);

    g_main_loop_run(loop);
}

void YDownloader2::onHeaders(SoupMessage* msg, YDownloader2* self)
{
    std::cout << "On headers" << std::endl;

    if(SOUP_STATUS_IS_SUCCESSFUL(soup_message_get_status(msg)))
    {
        SoupMessageHeaders* headers = soup_message_get_response_headers(msg);
        const goffset contentLength = soup_message_headers_get_content_length(headers);
        self->buffer.resize(contentLength);

        if(self->sizeCallback) self->sizeCallback(contentLength);

        std::cout << "Total size: " << self->totalSize << " bytes" << std::endl;
    }
    else
    {
        std::cerr << "Server error: " << soup_message_get_status(msg) << std::endl;
        if(msg) g_object_unref(msg);
    }
}

void YDownloader2::onResponse(GObject *, GAsyncResult *res, gpointer user_data)
{
    std::cout << "On response" << std::endl;

    auto* self = static_cast<YDownloader2*>(user_data);
    GError *error = nullptr;
    GInputStream *in = soup_session_send_finish(self->session, res, &error);

    if(error)
    {
        if(g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            std::cerr << "Download cancelled" << std::endl;
        }
        else
        {
            std::cerr << "Download failed: " << error->message << std::endl;
        }

        g_error_free(error);
        g_clear_object(&self->loop);

        return;
    }

    while (true)
    {
        gsize readBytes = 0;
        GError* readErr = nullptr;
        void* buffer = self->buffer.data() + self->dataOffset;

        g_input_stream_read_all(in, buffer, CHUNK_SIZE, &readBytes, self->cancellable, &readErr);

        if(readErr)
        {
            if(g_error_matches(readErr, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            {
                std::cerr << "Cancelled during download" << std::endl;
            }
            else
            {
                std::cerr << "Read error: " << readErr->message << std::endl;
            }

            g_error_free(readErr);
            break;
        }

        if (readBytes > 0)
        {
            if(self->totalSize > 0)
            {
                const auto percent = static_cast<uint8_t>(static_cast<double>(self->dataOffset) / self->totalSize * 100);
                std::cout << "Downloaded " << self->dataOffset << "/" << self->totalSize
                    << " (" << percent << "% )" << std::endl;
            }
            else
            {
                std::cout << "Downloaded " << self->dataOffset << " bytes" << std::endl;
            }

            self->dataOffset += readBytes;
            self->decrypt();

            if(self->cancellable && g_cancellable_is_cancelled(self->cancellable))
            {
                std::cerr << "Cancelled during download" << std::endl;
                break;
            }
        }
    }

    g_input_stream_close(in, nullptr, nullptr);
    std::cout << "Download completed." << std::endl;
    g_clear_object(&self->loop);
}

void YDownloader2::decrypt()
{
    if(!needDecryption)
    {
        _decryptOffset = dataOffset;
        return;
    }

    uint64_t dataSize = dataOffset - _decryptOffset;
    if(dataOffset < buffer.size())
    {
        const uint8_t sizeCorrection = dataSize % 16;
        if(sizeCorrection > 0) dataSize -= sizeCorrection;
    }

    yDec.decrypt(buffer.data() + _decryptOffset, dataSize);
    _decryptOffset += dataSize;
}

uint8_t YDownloader2::progress() const
{
    if(_decryptOffset == 0) return 0.f;
    if(_decryptOffset == buffer.size()) return 100.f;

    return 100 * _decryptOffset / buffer.size();
}

void YDownloader2::cancel()
{
    std::cout << "Cancellation requested" << std::endl;
    g_cancellable_cancel(cancellable);
}
