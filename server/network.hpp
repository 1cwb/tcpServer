#pragma once
#include "signal.h"
class NetWork
{
public:
    NetWork() {signal(SIGPIPE, SIG_IGN);}
};