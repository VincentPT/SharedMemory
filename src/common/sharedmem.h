#pragma once

// define sharing name in private session
#define APPLICATION_SESSION "SharedMemApp"
#define OBJECT_BOUNDARY "SharedMemAppBD"
#define SHARED_NAME "MyFileMappingObject"
#define BUFFER_READY_EVENT_NAME "BufferReady"
#define BUFFER_MUTEX_NAME "BufferAccessControl"

#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080
#define RAW_RGB_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT * 3)
#define MEM_SHARED_SIZE (sizeof(SharedVariables) + RAW_RGB_SIZE)

// define only simple variable here
// Do not define classes that require 'deep' copy constructors
// Don't use pointers because their address is not valid in other process
struct SharedVariables {
    unsigned char maxClientsFlags[8];
    unsigned int frameCount;
    HANDLE serverHandle;
};

class PrivateNameSpaceContext {
    void* _context;
public:
    PrivateNameSpaceContext(LPCTSTR szBoundaryName, LPCTSTR privateName);
    ~PrivateNameSpaceContext();

    // client can call either one of two functions below
    BOOL CreateContext();
    BOOL OpenContext();

    // client can explicit call this function to close the context or
    // this will auto close when the PrivateNameSpaceContext's object is destroyed
    BOOL CloseContext();
};