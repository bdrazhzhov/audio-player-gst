// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#include "y-downloader.h"

#include <cinttypes>
#include <iostream>
#include <libsoup/soup.h>

#define CHUNK_SIZE 4096

void YDownloader::start(const char* url, const char* key)
{
    dataOffset = 0;
    needDecryption = !!key;
    if (needDecryption) ydec.init(key);
    _decryptOffset = 0;

    if(worker && worker->joinable())
    {
        isRunning = false;
        worker->join();
    }

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
            std::cerr << "Connection error (retry " << connAttempts + 1
                << "): " << (error ? error->message : "unknown error") << std::endl;
            g_clear_error(&error);
            connAttempts++;
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
    gssize readBytes = -1;

    while(readAttempts < maxReadAttempts)
    {
        readBytes = g_input_stream_read(connection.stream, buffer.data() + dataOffset, CHUNK_SIZE, nullptr, &error);
        if(error)
        {
            std::cerr << "Read error (retry " << readAttempts + 1 << "): "
                << error->message << std::endl;
            g_clear_error(&error);
            readAttempts++;

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
