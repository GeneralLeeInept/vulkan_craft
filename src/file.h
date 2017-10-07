#pragma once

#include <stdint.h>
#include <stdio.h>

class File
{
public:
    ~File() { close(); }

    bool open(const std::wstring& path)
    {
        _wfopen_s(&_fp, path.c_str(), L"rb");

        if (_fp)
        {
            fseek(_fp, 0, SEEK_END);
            _length = ftell(_fp);
            fseek(_fp, 0, SEEK_SET);
        }

        return _fp != nullptr;
    }

    void close()
    {
        if (_fp)
        {
            fclose(_fp);
            _fp = nullptr;
        }
    }

    uint32_t get_length() { return _length; }

    uint32_t read(void* buffer, uint32_t bytes) { return (uint32_t)fread(buffer, 1, bytes, _fp); }

    operator FILE*() { return _fp; }
    operator bool() { return _fp != nullptr; }
    bool operator!() { return _fp == nullptr; }

private:
    FILE* _fp = nullptr;
    uint32_t _length = 0;
};
