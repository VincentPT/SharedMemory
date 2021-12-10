#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>

#include "../common/sharedmem.h"

class PrivateNameSpaceContextImpl {
    LPCTSTR _szBoundaryName;
    LPCTSTR _szPrivateName;
    HANDLE _boundaryDescriptor = NULL;
    HANDLE _hNameSpace = NULL;

public:
    PrivateNameSpaceContextImpl(LPCTSTR szBoundaryName, LPCTSTR szPrivateName) : _szBoundaryName(szBoundaryName), _szPrivateName(szPrivateName) {
    }

    ~PrivateNameSpaceContextImpl() {
        CloseContext();
    }

        // client can call either one of two functions below
    BOOL CreateContext() {
        CreatePrivateNameSpace(_szBoundaryName, _szPrivateName);
        return _hNameSpace != NULL;
    }

    BOOL OpenContext() {
        OpenPrivateNameSpace(_szBoundaryName, _szPrivateName);
        return _hNameSpace != NULL;
    }

    // client can explicit call this function to close the context or
    // this will auto close when the PrivateNameSpaceContext's object is destroyed
    BOOL CloseContext() {
        if(_boundaryDescriptor != NULL)
        {
            DeleteBoundaryDescriptor(_boundaryDescriptor);
            _boundaryDescriptor = NULL;
        }

        if(_hNameSpace != NULL)
        {
            ClosePrivateNamespace(_hNameSpace, PRIVATE_NAMESPACE_FLAG_DESTROY);
            _hNameSpace = NULL;
        }

        return TRUE;
    }

private:
    void CreateBoundaryDescriptorForPrivateNameSpace(LPCTSTR szBDName) {
        DWORD dwError = ERROR_SUCCESS;
        BOOL bRet = FALSE;
        TCHAR szDomainName[MAX_PATH + 1] = {0};
        DWORD cchDomainName = MAX_PATH + 1;
        PSID pSid = NULL;
        BYTE Sid[SECURITY_MAX_SID_SIZE];
        DWORD cbSid = SECURITY_MAX_SID_SIZE;
        SID_NAME_USE sidNameUse;

        TCHAR currentUser[64];
        DWORD bufferSize = sizeof(currentUser);
        if(!GetUserName(currentUser, &bufferSize)) {
            _tprintf(TEXT("Could not get user name (%d).\n"),
                GetLastError());
            return;
        }
        
        // Get the Service SID
        pSid = (PSID) Sid;
        bRet = LookupAccountName(NULL, currentUser, pSid, &cbSid, szDomainName, &cchDomainName, &sidNameUse);
        if(!bRet)
        {
            dwError = GetLastError();
            _tprintf(TEXT("Error looking up service SID (%d)\n"), dwError);
            return;
        }

        // Create the Boundary Descriptor for the private namespace
        _boundaryDescriptor = CreateBoundaryDescriptor(szBDName, 0);
        if(_boundaryDescriptor == NULL)
        {
            dwError = GetLastError();
            _tprintf(TEXT("Error creating Boundary Descriptor for private namespace (%d)"), dwError);
            return;
        }

        // Add the Service SID to the Boundary Descriptor
        bRet = AddSIDToBoundaryDescriptor(&_boundaryDescriptor, pSid);
        if(!bRet)
        {
            dwError = GetLastError();
            _tprintf(TEXT("Error adding SID to Boundary Descriptor  (%d)"), dwError);
            return;
        }
    }

    void CreatePrivateNameSpace(LPCTSTR szBDName, LPCTSTR privateNameSpace) {
        CreateBoundaryDescriptorForPrivateNameSpace(szBDName);

        if(_boundaryDescriptor) {
            // Create the private namespace
            _hNameSpace = CreatePrivateNamespace(NULL, _boundaryDescriptor, privateNameSpace); 
            if(_hNameSpace == NULL)
            {
                _tprintf(TEXT("Error creating private namespace  (%d)"), GetLastError());
                return;
            }
        }
    }

    void OpenPrivateNameSpace(LPCTSTR szBDName, LPCTSTR privateNameSpace) {
        CreateBoundaryDescriptorForPrivateNameSpace(szBDName);

        if(_boundaryDescriptor) {
            // Open the private namespace
            _hNameSpace = OpenPrivateNamespace(_boundaryDescriptor, privateNameSpace); 
            if(_hNameSpace == NULL)
            {
                _tprintf(TEXT("Error creating private namespace  (%d)"), GetLastError());
                return;
            }
        }
    }
};

#define NAME_SPACE_CONTEXT() ((PrivateNameSpaceContextImpl*)_context)

PrivateNameSpaceContext::PrivateNameSpaceContext(LPCTSTR szBoundaryName, LPCTSTR privateName) {
    _context = new PrivateNameSpaceContextImpl(szBoundaryName, privateName);
}

PrivateNameSpaceContext::~PrivateNameSpaceContext() {
    delete NAME_SPACE_CONTEXT();
}

// client can call either one of two functions below
BOOL PrivateNameSpaceContext::CreateContext() {
    return NAME_SPACE_CONTEXT()->CreateContext();
}

BOOL PrivateNameSpaceContext::OpenContext() {
    return NAME_SPACE_CONTEXT()->OpenContext();
}

// client can explicit call this function to close the context or
// this will auto close when the PrivateNameSpaceContext's object is destroyed
BOOL PrivateNameSpaceContext::CloseContext() {
    return NAME_SPACE_CONTEXT()->CloseContext();
}