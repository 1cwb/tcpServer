#pragma once
#include <iostream>
#include <vector>
#include <mutex>

class Buffer
{
public:
    explicit Buffer(std::size_t size = 1024)
        : _buffer(size), _readIndex(0), _writeIndex(0) {}
    Buffer (const Buffer& buf) : Buffer() {Write(buf);}

    const char* readPos() const { return _buffer.data() + _readIndex; }
    std::size_t readableSize() const { return _writeIndex - _readIndex; }
    std::size_t writeableSize() const {return backsize() + frontSize(); }
    bool moveReadIdx(std::size_t len)
    { 
        if (_readIndex + len > _writeIndex)
        {
            return false;
        }
        _readIndex += len;
        return true;
    }
    bool moveWriteIdx(std::size_t len)
    {
        if(len > backsize())
        {
            return false;
        }
        _writeIndex += len;
        return true;
    }
    void read(void* buff, std::size_t len, bool pop = false)
    {
        if(len > readableSize() || len == 0)
        {
            return;
        }
        std::copy(readPos(), static_cast<const char*>(readPos() + len), static_cast<char*>(buff));
        if(pop)
        {
            moveReadIdx(len);
        }
    }
    std::string readAsString(std::size_t len, bool pop = false)
    {
        if(len > readableSize() || len == 0)
        {
            return std::string();
        }
        std::string str(readPos(), len);
        if(pop)
        {
            moveReadIdx(len);
        }
        return str;
    }
    std::string readLine(bool pop = false)
    {
        char* crlf = findCRLF();
        if(crlf == nullptr)
        {
            return std::string();
        }
        std::string str(readPos(), crlf - readPos() + 1);
        if(pop)
        {
            moveReadIdx(str.size());
        }
        return str;
    }
    void write(const void* data, std::size_t len, bool push = true)
    {
        if(len == 0)
        {
            return;
        }
        ensureWriteable(len);
        std::copy(static_cast<const char*>(data), static_cast<const char*>(data) + len, writePos());
        if(push)
        {
            moveWriteIdx(len);
        }
    }
    void write(const std::string& str, bool push = true)
    {
        write(str.data(), str.size(), push);
    }
    void Write(const Buffer& buf, bool push = true)
    {
        write(buf.readPos(), buf.readableSize(), push);
    }
    void clear()
    {
        _readIndex = 0;
        _writeIndex = 0;
    }
private:
    char* writePos() {return _buffer.data() + _writeIndex;}
    std::size_t frontSize() const {return _readIndex;}
    std::size_t backsize() const {return _buffer.size() - _writeIndex;}

    void ensureWriteable(std::size_t len)
    {
        if(len > backsize())
        {
            if(len > writeableSize())
            {
                _buffer.resize(_buffer.size() + len);
            }
            else
            {
                std::copy(readPos(), static_cast<const char*>(writePos()), _buffer.data());
                _writeIndex -= frontSize();
                _readIndex = 0;
            }
        }

    }
    char* findCRLF()
    {
        for(auto p = readPos(); p != writePos(); p++)
        {
            if(*p == '\n')
            {
                return const_cast<char*>(p);
            }
        }
        return nullptr;
    }
private:
    std::vector<char> _buffer;
    std::size_t _readIndex;
    std::size_t _writeIndex;
};