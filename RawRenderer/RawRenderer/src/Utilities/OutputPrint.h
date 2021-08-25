#pragma once

#include <Windows.h>
#include <sstream>

#define PRINTOUT( s )            \
{                             \
   std::wstringstream os_;    \
   os_ << s;                   \
   OutputDebugString( os_.str().c_str() );  \
}