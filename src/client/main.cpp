#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <chrono>

#include "../common/sharedmem.h"

int main(int argc, char *argv[])
{
    HANDLE hMapFile;
    LPCTSTR pBuf;

    PrivateNameSpaceContext namespaceContext(TEXT(OBJECT_BOUNDARY), TEXT(APPLICATION_SESSION));
    if(!namespaceContext.OpenContext()) {
        return 1;
    }

    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS, // read/write access
        FALSE,               // do not inherit the name
        SHARED_NAME);             // name of mapping object

    if (hMapFile == NULL)
    {
        _tprintf(TEXT("Could not open file mapping object (%d).\n"),
                 GetLastError());
        return 1;
    }

    pBuf = (LPTSTR)MapViewOfFile(hMapFile,            // handle to map object
                                 FILE_MAP_ALL_ACCESS, // read/write permission
                                 0,
                                 0,
                                 MEM_SHARED_SIZE);

    if (pBuf == NULL)
    {
        _tprintf(TEXT("Could not map view of file (%d).\n"),
                 GetLastError());

        CloseHandle(hMapFile);

        return 1;
    }

    // create synchroniazation objects
    HANDLE bufferReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, BUFFER_READY_EVENT_NAME);
    if(bufferReadyEvent == NULL) {
        _tprintf(TEXT("Could not open event object (%d).\n"),
                 GetLastError());

        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);

        return 1;
    }

    HANDLE bufferAccessControl = OpenMutex(SYNCHRONIZE, FALSE, BUFFER_MUTEX_NAME);
    if(bufferReadyEvent == NULL) {
        _tprintf(TEXT("Could not open mutex object (%d).\n"),
                 GetLastError());
        CloseHandle(bufferReadyEvent);
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);

        return 1;
    }

    SharedVariables* bufferHeader = (SharedVariables*)pBuf;
    char* data = ((char*) pBuf + sizeof(SharedVariables));
    unsigned char& frameChanged = bufferHeader->maxClientsFlags[0];

    HANDLE serverHandle = bufferHeader->serverHandle;

    DWORD waitResult = WaitForSingleObject(bufferReadyEvent, INFINITE);
    double framePerSec = 0;
    if(waitResult == WAIT_OBJECT_0) {
        auto startTime = std::chrono::system_clock::now();
        int frameCount = 0;
        while(true) {
            if(frameChanged) {
                waitResult = WaitForSingleObject(bufferAccessControl, INFINITE);
                if(waitResult == WAIT_OBJECT_0) {
                    _tprintf(TEXT("Share memory read: %d %s %.02f\n"), bufferHeader->frameCount, data, framePerSec);
                    frameChanged = 0;
                    ReleaseMutex(bufferAccessControl);

                    frameCount++;
                    if(frameCount > 1) {
                        auto now = std::chrono::system_clock::now();
                        auto durantion = now - startTime;
                        auto durationSecond = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1,1>>>(durantion);

                        framePerSec = frameCount / durationSecond.count();
                    }
                }
            }
            else {
                //_tprintf(TEXT("Frame is not ready\n"));
                //Sleep(1);
            }
        }
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}
