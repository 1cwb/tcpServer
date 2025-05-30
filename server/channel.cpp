#include "channel.hpp"
#include "eventloop.hpp"
void Channel::update()
{
    _loop->updateEvent(this);
}
void Channel::remove()
{
    _loop->removeEvent(this);
}
