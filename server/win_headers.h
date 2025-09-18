#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H

// Block old winsock
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#define WIN32_LEAN_AND_MEAN

// Include in correct order
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mswsock.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#endif