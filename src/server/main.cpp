#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>

#include "../common/sharedmem.h"

int main(int argc, char *argv[])
{
    HANDLE hMapFile;
    PVOID pBuf;

    PrivateNameSpaceContext namespaceContext(TEXT(OBJECT_BOUNDARY), TEXT(APPLICATION_SESSION));
    if(!namespaceContext.CreateContext()) {
        return 1;
    }

    // create mapping file
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, // use paging file
        NULL,                 // default security
        PAGE_READWRITE,       // read/write access
        0,                    // maximum object size (high-order DWORD)
        MEM_SHARED_SIZE,         // maximum object size (low-order DWORD)
        SHARED_NAME);         // name of mapping object

    if (hMapFile == NULL)
    {
      _tprintf(TEXT("Could not create file mapping object (%d).\n"),
             GetLastError());
      return 1;
    }

    // get shared memory from mapping files
    pBuf = MapViewOfFile(hMapFile,            // handle to map object
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
    HANDLE bufferReadyEvent = CreateEvent(NULL, TRUE, FALSE, BUFFER_READY_EVENT_NAME);
    if(bufferReadyEvent == NULL) {
        _tprintf(TEXT("Could not create event object (%d).\n"),
                 GetLastError());

        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);

        return 1;
    }

    HANDLE bufferAccessControl = CreateMutex(NULL, FALSE, BUFFER_MUTEX_NAME);
    if(bufferReadyEvent == NULL) {
        _tprintf(TEXT("Could not create mutex object (%d).\n"),
                 GetLastError());
        CloseHandle(bufferReadyEvent);
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);

        return 1;
    }
    
    // generate data
    char* data = new char[RAW_RGB_SIZE];
    {
        const char header[] = "HELLO CLIENT";
        memcpy_s(data, RAW_RGB_SIZE, header, sizeof(header));
        char* c = data + sizeof(header);
        char* end = data + RAW_RGB_SIZE;
        srand( (unsigned)time( NULL ) );

        while( c < end) {
            *c++ = (char)(rand() * 0xFF / RAND_MAX);
        }
    }
    _tprintf(TEXT("Press any key to set data\n"));
    _getch();
    SharedVariables* bufferHeader = (SharedVariables*)pBuf;
    // copy data to shared memory
    DWORD waitResult = WaitForSingleObject(bufferAccessControl, INFINITE);
    if(waitResult == WAIT_OBJECT_0) {
        memset(bufferHeader, 0, sizeof(SharedVariables));

        CopyMemory(((char*)pBuf + sizeof(SharedVariables)), data, RAW_RGB_SIZE);

        // mark all client flags that the frame is changed
        memset(bufferHeader->maxClientsFlags, 1, sizeof(bufferHeader->maxClientsFlags));

        SetEvent(bufferReadyEvent);
        ReleaseMutex(bufferAccessControl);

        _tprintf(TEXT("Transfer data 0\n"));
    }

    for(int i = 0; i < 100000; ++i)
    {
        waitResult = WaitForSingleObject(bufferAccessControl, INFINITE);
        if(waitResult != WAIT_OBJECT_0) {
            continue;
        }
        bufferHeader->frameCount = (i + 1);
        CopyMemory(((char*)pBuf + sizeof(SharedVariables)), data, RAW_RGB_SIZE);

        // mark all client flags that the frame is changed
        memset(bufferHeader->maxClientsFlags, 1, sizeof(bufferHeader->maxClientsFlags));

        SetEvent(bufferReadyEvent);
        ReleaseMutex(bufferAccessControl);
        _tprintf(TEXT("Transfer data %d\n"), bufferHeader->frameCount);
    }
    

    _tprintf(TEXT("Press any key to exit the program\n"));
    _getch();

    delete []data;

    CloseHandle(bufferReadyEvent);
    CloseHandle(bufferAccessControl);
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    return 0;
}
