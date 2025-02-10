#include "pch.h"
#include "EtherSocket.h"

#include <vector>
#include <winsock2.h>   // For htonl(), etc.
#include <cstring>      // For memcpy

void CEtherSocket::OnConnect(int nErrorCode)
{
    if (m_pParent) {
        ::PostMessage(m_pParent->GetSafeHwnd(), WM_SOCKET_CONNECTED, (WPARAM)nErrorCode, 0);
    }
    CAsyncSocket::OnConnect(nErrorCode);
}

void CEtherSocket::OldOnReceive(int nErrorCode)
{
    if (nErrorCode != 0) {
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // Receive up to 1023 bytes.
    char buffer[1024];
    int nBytes = Receive(buffer, sizeof(buffer) - 1);
    if (nBytes > 0) {
        buffer[nBytes] = '\0'; // Ensure null termination
        CString strReceived(buffer);
        if (m_pParent) {
            // Post a message to the dialog with the received text.
            ::PostMessage(m_pParent->GetSafeHwnd(), WM_SOCKET_RECEIVE, 0, (LPARAM)new CString(strReceived));
        }
    }
    CAsyncSocket::OnReceive(nErrorCode);
}

void CEtherSocket::OnReceive(int nErrorCode)
{
    if (nErrorCode != 0)
    {
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // Receive data into a local buffer.
    char buffer[1024];
    int nBytes = Receive(buffer, sizeof(buffer));
    if (nBytes <= 0)
    {
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // Ensure that we have at least the minimal packet size (16 bytes).
    if (nBytes < 16)
    {
        // Incomplete or invalid packet; wait for more data.
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // --- Parse the packet ---
    // 1. Start marker (first 4 bytes)
    uint32_t netStartMarker = 0;
    memcpy(&netStartMarker, buffer, sizeof(netStartMarker));
    uint32_t startMarker = ntohl(netStartMarker);
    if (startMarker != 0xBAADF00D)
    {
        // Invalid start marker.
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // 2. Packet length (next 4 bytes)
    uint32_t netPacketLength = 0;
    memcpy(&netPacketLength, buffer + 4, sizeof(netPacketLength));
    uint32_t packetLength = ntohl(netPacketLength);
    if (packetLength > static_cast<uint32_t>(nBytes))
    {
        // The complete packet has not yet been received.
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // 3. Message index (next 4 bytes)
    uint32_t netMsgIndex = 0;
    memcpy(&netMsgIndex, buffer + 8, sizeof(netMsgIndex));
    uint32_t msgIndex = ntohl(netMsgIndex);

    // 4. Message content: its length is (packetLength - 16)
    uint32_t payloadLength = packetLength - 16;
    CStringA messageA;
    if (payloadLength > 0)
    {
        // Allocate a temporary buffer for the payload and ensure null termination.
        char* pPayload = new char[payloadLength + 1];
        memcpy(pPayload, buffer + 12, payloadLength);
        pPayload[payloadLength] = '\0';
        messageA = pPayload;
        delete[] pPayload;
    }
    else
    {
        messageA = "";
    }

    // 5. End marker (last 4 bytes)
    uint32_t netEndMarker = 0;
    memcpy(&netEndMarker, buffer + 12 + payloadLength, sizeof(netEndMarker));
    uint32_t endMarker = ntohl(netEndMarker);
    if (endMarker != 0xDEADBEEF)
    {
        // Invalid end marker.
        CAsyncSocket::OnReceive(nErrorCode);
        return;
    }

    // Convert the ASCII message to a Unicode CString.
    CString strMessage(messageA);

    // Post the received message to the parent dialog's log window.
    if (m_pParent)
    {
        ::PostMessage(m_pParent->GetSafeHwnd(), WM_SOCKET_RECEIVE, 0, (LPARAM)new CString(strMessage));
    }

    CAsyncSocket::OnReceive(nErrorCode);
}

void CEtherSocket::OnSend(int nErrorCode) {
    CAsyncSocket::OnSend(nErrorCode);
}

void CEtherSocket::OnClose(int nErrorCode)
{
    if (m_pParent) {
        ::PostMessage(m_pParent->GetSafeHwnd(), WM_SOCKET_CLOSED, (WPARAM)nErrorCode, 0);
    }
    CAsyncSocket::OnClose(nErrorCode);
}

bool CEtherSocket::IsConnected() const {
    if (m_hSocket == INVALID_SOCKET)
        return false;
    int error = 0;
    int len = sizeof(error);
    if (getsockopt(m_hSocket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) == SOCKET_ERROR) {
        return false;
    }
    return (error == 0);
}


void CEtherSocket::SendAsMessage(const CString& str)
{
    // Use a static variable to keep track of the message index.
    // Alternatively, you could use a member variable.
    static uint32_t messageIndex = 1;

    // Convert the CString to an ANSI (ASCII) string.
    // (If your protocol is truly ASCII you should ensure your CString contains only ASCII.)
    CStringA asciiStr(str);
    int messageLength = asciiStr.GetLength();

    // Total packet length: 4 (start) + 4 (length) + 4 (index) + messageLength + 4 (end) = 16 + messageLength
    uint32_t totalPacketLength = 16 + messageLength;

    // Create a buffer to hold the packet.
    std::vector<char> packet(totalPacketLength);

    // Define the protocol markers.
    const uint32_t START_MARKER = 0xBAADF00D;
    const uint32_t END_MARKER = 0xDEADBEEF;

    // Convert numbers to network (big-endian) byte order.
    uint32_t netStart = htonl(START_MARKER);
    uint32_t netLength = htonl(totalPacketLength);
    uint32_t netMsgIndex = htonl(messageIndex);
    uint32_t netEnd = htonl(END_MARKER);

    // Copy the fields into the packet buffer.
    // Start marker at offset 0.
    memcpy(packet.data(), &netStart, sizeof(netStart));
    // Packet length at offset 4.
    memcpy(packet.data() + 4, &netLength, sizeof(netLength));
    // Message index at offset 8.
    memcpy(packet.data() + 8, &netMsgIndex, sizeof(netMsgIndex));
    // Message content at offset 12.
    memcpy(packet.data() + 12, asciiStr.GetBuffer(), messageLength);
    // End marker at offset 12 + messageLength.
    memcpy(packet.data() + 12 + messageLength, &netEnd, sizeof(netEnd));

    // Increment the message index for the next message.
    messageIndex++;

    // Send the packet. The Send function is inherited from CAsyncSocket.
    int bytesSent = Send(packet.data(), static_cast<int>(packet.size()));
    if (bytesSent == SOCKET_ERROR)
    {
        // Handle the error (e.g. log it or notify the user)
        // You might use GetLastError() for more details.
    }
}


