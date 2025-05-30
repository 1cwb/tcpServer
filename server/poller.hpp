#pragma once
#include <sys/epoll.h>
#include <vector>
#include <unordered_map>
#include "channel.hpp"
class Poller
{
public:
    Poller() : _epollfd(epoll_create1(EPOLL_CLOEXEC)), _events(1024)
    {
        if(_epollfd == -1)
        {
            perror("epoll_create1");
            exit(EXIT_FAILURE);
        }
    }
    ~Poller()
    {
        close(_epollfd);
    }

    void update(Channel* ch)
    {
        if(hasChannel(ch->getFd()))
        {
            epollOp(EPOLL_CTL_MOD, ch);
        }
        else
        {
            epollOp(EPOLL_CTL_ADD, ch);
            _channels[ch->getFd()] = ch;
        }
    }
    void remove(Channel* ch)
    {
        auto it = _channels.find(ch->getFd());
        if(it != _channels.end())
        {
            epollOp(EPOLL_CTL_DEL, ch);
            _channels.erase(it);
        }
    }
    bool poll(std::vector<Channel*>& activeChannels, int timeout = -1)
    {
        int n = epoll_wait(_epollfd, _events.data(), _events.size(), timeout);
        if(n == -1)
        {
            if(errno == EINTR)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        for(int i = 0; i < n; i++)
        {
            auto ch = _channels.find(_events[i].data.fd);
            if(ch == _channels.end())
            {
                return false;
            }
            ch->second->setRevents(_events[i].events);
            activeChannels.push_back(ch->second);
        }
        return true;
    }

private:
    void epollOp(int op, Channel* ch)
    {
        int fd = ch->getFd();
        epoll_event ev{};
        ev.data.fd = fd;
        ev.events = ch->getEvents();
        int ret = epoll_ctl(_epollfd, op, fd, &ev);
        if(ret == -1)
        {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
    }
    bool hasChannel(int fd) const { return _channels.find(fd) != _channels.end(); }
private:
    int _epollfd;
    std::vector<epoll_event> _events;
    std::unordered_map <int, Channel*> _channels;
};