//
// Created by boris on 01.08.23.
//
#pragma once
#include <stdexcept>

class AudioPlayerException : public std::runtime_error
{
public:
    explicit AudioPlayerException(const char* msg) : runtime_error(msg){}
    explicit AudioPlayerException(const std::string& msg) : runtime_error(msg){}
};
