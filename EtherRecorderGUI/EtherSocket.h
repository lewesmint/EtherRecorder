#pragma once
#include <afxsock.h>

// Define custom messages for socket events.
#define WM_SOCKET_CONNECTED (WM_USER + 100)
#define WM_SOCKET_RECEIVE   (WM_USER + 101)
#define WM_SOCKET_CLOSED    (WM_USER + 102)  // Custom message for disconnect

class CEtherSocket : public CAsyncSocket {
public:
    CEtherSocket() : m_pParent(nullptr) {}
    virtual ~CEtherSocket() {
        if (m_hSocket != INVALID_SOCKET)
            Close();
    }

    // Pointer to the parent window (our dialog) so that we can update the UI.
    CWnd* m_pParent;

    virtual void OnConnect(int nErrorCode);
	virtual void OldOnReceive(int nErrorCode); // now used in this application
    virtual void OnReceive(int nErrorCode);
    virtual void OnClose(int nErrorCode);  // Added OnClose handler
    virtual void OnSend(int nErrorCode);
    
    
    
	virtual void SendAsMessage(const CString& str);


	// Helper function to send data to the server.
    bool IsConnected() const;
};
