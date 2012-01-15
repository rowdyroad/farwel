/*! @file       MemCacheClient.cpp
 *  @version    1.2
 *  @brief      Basic memcached client
 */
//#include "stdafx.h"

/*! @cond IGNORE */
#ifdef WIN32
# pragma warning(disable: 4127 4702) // C4127: conditional expression is constant, C4702: unreachable code
# pragma comment(lib, "ws2_32.lib")
# include <winsock2.h>
# define GetLastSocketError()    WSAGetLastError()
# define EWOULDBLOCK       WSAEWOULDBLOCK
# define snprintf          _snprintf
# define SPRINTF_UINT64    "%I64u"
# define STRTOUL64         _strtoui64
#else
# include <unistd.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <errno.h>
# define SOCKET            int
# define INVALID_SOCKET    -100
# define SOCKET_ERROR      -200
# define closesocket       close
# define ioctlsocket       ioctl
# define GetLastSocketError()    errno
# ifndef EWOULDBLOCK
#  define EWOULDBLOCK      EAGAIN
# endif
# define SPRINTF_UINT64    "%llu"
# define STRTOUL64         strtoull
#endif
/*! @endcond */

#include <stdio.h>
#include <string.h>

#include <vector>
#include <algorithm>
#include <cassert>

#include "MemCacheClient.h"
#include "sha1.h"

/*! @brief Minimum period of time between connection attempts to a server.
 *
 *  After either an attempt or an actual connection to a server, there will
 *  be a minimum of this period before we attempt to connect again.
 */
#define MEMCACHECLIENT_RECONNECT_SEC    60

///////////////////////////////////////////////////////////////////////////////
// ServerSocket
//

/*! @brief Socket connection, disconnection, and buffered data receives. */
class ServerSocket
{
    private:

        class ConnectException
            : public std::exception
        {
            private:
                int         rc_;
                std::string msg_;
            public:
                ConnectException(int rc, const std::string& msg) throw()
                    : rc_(rc)
                    , msg_(msg)
                {}

                ~ConnectException() throw()
                {}
                const char *what() const throw()
                {
                    return msg_.c_str();
                }

                const int code() const
                {
                    return rc_;
                }
        };
        const static int MAXBUF = 1024; //!< size of internal data buffer

        SOCKET mSocket;                 //!< socket being abstracted
        char   mBuf[MAXBUF];            //!< internal data buffer
        int    mIdx;                    //!< current read index for mBuf
        int    mBufLen;                 //!< current end index for mBuf

    private:
        /*! @brief copy constructor is disabled */
        ServerSocket(const ServerSocket&);
        /*! @brief copy operator is disabled */
        ServerSocket& operator=(const ServerSocket&);

        /*! @brief Receive data into a buffer with as many bytes as possible up
         *  to the size of the buffer.
         *  @param a_pszBuf     Buffer to be filled
         *  @param a_nBufSiz    Maximum number of bytes to receive
         *  @return number of bytes actually received
         */
        int ReceiveBytes(char *a_pszBuf, int a_nBufSiz);

    public:
        /*! @brief Exception thrown on any send or receive error */
        class Exception
            : public std::exception
        {
            public:
                const char *mWhat; //!< error message

                /*! constructor
                 *  @param aWhat optional message
                 */
                Exception(const char *aWhat = "") { mWhat = aWhat; }
        };

    public:
        /*! @brief constructor */
        ServerSocket();

        /*! @brief destructor */
        ~ServerSocket();

        /*! @brief connect the socket to an address
         *  @param a_nIpAddress Server IP address as returned from inet_addr
         *  @param a_nPort      Server port
         *  @param a_nTimeout   Connection timeout period in milliseconds
         */
        bool Connect(unsigned long a_nIpAddress, int a_nPort, int a_nTimeout);

        /*! @brief Determine if we are currently connected to a server */
        inline bool IsConnected() const { return mSocket != INVALID_SOCKET;  }

        /*! @brief Disconnect from the server */
        void Disconnect();

        /*! @brief Send the supplied bytes to the server.
         *
         *  A blocking send is used, so the function will block until either
         *  all bytes are sent or an error occurs.
         *
         *  @param a_pszBuf     Bytes to send to the server
         *  @param a_nBufSiz    Number of bytes to send to the server
         *  @throw Exception on socket error
         */
        void SendBytes(const char *a_pszBuf, size_t a_nBufSiz); // throw Exception

        /*! @brief Receive a block of data from the server.
         *  @param a_pszBuf     Buffer to receive the data into
         *  @param a_nBufSiz    Maximum number of bytes to be received
         *  @return number of bytes that were actually received
         *  @throw Exception on socket error
         */
        int GetBytes(char *a_pszBuf, int a_nBufSiz); // throw Exception

        /*! @brief Receive and discard bytes.
         *  @param a_nBytes Number of bytes to be received and ignored.
         *  @throw Exception on socket error
         */
        void DiscardBytes(int a_nBytes); // throw Exception

        /*! @brief Receive a single byte
         *  @return received byte
         *  @throw Exception on socket error
         */
        inline char GetByte()
        {
            if (mIdx >= mBufLen) {
                mIdx    = 0;
                mBufLen = ReceiveBytes(mBuf, MAXBUF);
            }
            return mBuf[mIdx++];
        }
};

ServerSocket::ServerSocket()
    : mSocket(INVALID_SOCKET)
    , mIdx(0)
    , mBufLen(0)
{ }

ServerSocket::~ServerSocket()
{
    Disconnect();
}

void ServerSocket::Disconnect()
{
    if (mSocket == INVALID_SOCKET) {
        return;
    }

    // shutdown SD_SEND
    shutdown(mSocket, 1);

    int nTimeout = 10;
    setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&nTimeout, sizeof(nTimeout));

    // read all pending data
    int rc = 1;
    while (rc != SOCKET_ERROR && rc > 0) {
        rc = recv(mSocket, mBuf, MAXBUF, 0);
    }

    // done
    closesocket(mSocket);
    mSocket = INVALID_SOCKET;

    // clear the buffer
    mIdx = mBufLen = 0;
}

bool ServerSocket::Connect(
    unsigned long a_nIpAddress,
    int           a_nPort,
    int           a_nTimeout
)
{
    Disconnect();

    struct sockaddr_in server;
    server.sin_family      = AF_INET;
    server.sin_port        = htons((short)a_nPort);
    server.sin_addr.s_addr = a_nIpAddress;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { return false;  }

    try {
        // non-blocking for connect
        u_long value = 1;
        int    rc    = ioctlsocket(s, FIONBIO, &value);
        if (rc != 0) { throw ServerSocket::ConnectException(rc, "Ioctrlsocket FIONBIO = 1"); }

        rc = connect(s, (struct sockaddr *)&server, sizeof(server));
        if (rc != 0) {
            if ((rc != SOCKET_ERROR) || (GetLastSocketError() != EWOULDBLOCK)) { throw ServerSocket::ConnectException(rc, "Connect error"); }

            // non-blocking wait
            struct timeval timeout;
            timeout.tv_sec  = a_nTimeout / 1000;
            timeout.tv_usec = (a_nTimeout % 1000) * 1000;
            fd_set wr;
            FD_ZERO(&wr);
            FD_SET(s, &wr);
            fd_set ex;
            FD_ZERO(&ex);
            FD_SET(s, &ex);
            rc = select(0, NULL, &wr, &ex, &timeout);
            if ((rc == 0) || (rc == SOCKET_ERROR) || FD_ISSET(s, &ex)) { throw ServerSocket::ConnectException(rc, "Select error"); }
        }

        // blocking
        value = 0;
        rc    = ioctlsocket(s, FIONBIO, &value);
        if (rc != 0) { throw ServerSocket::ConnectException(rc, "Ioctrlsocket FIONBIO = 0"); }

        rc = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
                        (const char *)&a_nTimeout, sizeof(a_nTimeout));
        //    if (rc != 0) throw ServerSocket::ConnectException(rc, "setsockopt RCVTIME");

        rc = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO,
                        (const char *)&a_nTimeout, sizeof(a_nTimeout));
        //    if (rc != 0) throw ServerSocket::ConnectException(rc, "setsockopt SNDIME");

        mSocket = s;
        return true;
    }catch (const ServerSocket::ConnectException& e) {
        closesocket(s);
        return false;
    }
}

void ServerSocket::SendBytes(
    const char *a_pszBuf,
    size_t     a_nBufSiz
)
{
    // blocking send, will return the number of bytes sent or
    // it is an error
    size_t n = send(mSocket, a_pszBuf, (int)a_nBufSiz, 0);

    if (n == a_nBufSiz) { return; }

    // on error disconnect and throw SocketException
    Disconnect();
    throw ServerSocket::Exception("send error");
}

int ServerSocket::ReceiveBytes(
    char *a_pszBuf,
    int  a_nBufSiz
)
{
    int n = recv(mSocket, a_pszBuf, a_nBufSiz, 0);

    if (n > 0) { return n;  }

    // on error disconnect and throw SocketException
    Disconnect();
    throw ServerSocket::Exception("recv error");
}

int ServerSocket::GetBytes(
    char *a_pszBuf,
    int  a_nBufSiz
)
{
    if (mIdx < mBufLen) {
        int nLen = mBufLen - mIdx;
        if (nLen > a_nBufSiz) { nLen = a_nBufSiz; }
        memcpy(a_pszBuf, mBuf + mIdx, nLen);
        mIdx += nLen;
        if (mIdx == mBufLen) { mIdx = mBufLen = 0; }
        return nLen;
    }
    return ReceiveBytes(a_pszBuf, a_nBufSiz);
}

void ServerSocket::DiscardBytes(
    int a_nBytes
)
{
    while (a_nBytes > 0) {
        if (mIdx == mBufLen) {
            mIdx    = 0;
            mBufLen = ReceiveBytes(mBuf, MAXBUF);
        }

        int nLen = mBufLen - mIdx;
        if (nLen > a_nBytes) {
            mIdx += a_nBytes;
            break;
        }

        a_nBytes -= nLen;
        mIdx      = mBufLen;
    }
}

///////////////////////////////////////////////////////////////////////////////

/*! @brief Abstraction of a server connection */
class MemCacheClient::Server
    : public ServerSocket
{
    /*! @brief max length of a server address */
    const static size_t ADDRLEN = sizeof("aaa.bbb.ccc.ddd:PPPPP");

    public:
        /*! @brief constructor */
        Server()
            : mIp(INADDR_NONE), mPort(0), mLastConnect(0) { mAddress[0] = 0; }

        /*! @brief copy constructor
         *  @param rhs server to copy address of
         */
        Server(const Server& rhs) { operator=(rhs); }

        /*! @brief destructor */
        ~Server() { }

        /*! @brief copy operator
         *  @param rhs server to copy server listen address from
         */
        Server& operator=(const Server& rhs);

        /*! @brief equality based on server listen address
         *  @param rhs server to compare to
         *  @return true when servers are using the same listen address
         */
        bool operator==(const Server& rhs) const;

        /*! @brief inequality based on server listen address
         *  @param rhs server to compare to
         *  @return true when servers are not using the same listen address
         */
        inline bool operator!=(const Server& rhs) const { return !operator==(rhs);  }

        /*! @brief Set the listen address for this server
         *  @param a_pszServer Listen address in IP:PORT format
         *  @return true if listen address was correctly parsed
         */
        bool Set(const char *a_pszServer);

        /*! @brief Attempt to connect to the server.
         *  @param a_nTimeout Timeout for connection in milliseconds
         *  @return true if connection was successful
         */
        bool Connect(int a_nTimeout);

        /*! @brief Get the string representation of this server listen address.
         *
         *  This may not be the same as the address passed in. It will always be
         *  in the format IP:PORT.
         *
         *  @return Server listen address as IP:PORT
         */
        inline const char *GetAddress() const { return mAddress;  }

    private:
        char          mAddress[ADDRLEN]; //!< server IP:PORT as string
        unsigned long mIp;               //!< server IP address
        int           mPort;             //!< server port
        time_t        mLastConnect;      //!< last connection time
};

MemCacheClient::Server& MemCacheClient::Server::operator=(
    const Server& rhs
)
{
    strcpy(mAddress, rhs.mAddress);
    mIp          = rhs.mIp;
    mPort        = rhs.mPort;
    mLastConnect = 0;
    return *this;
}

bool MemCacheClient::Server::operator==(
    const Server& rhs
) const
{
    return mIp == rhs.mIp && mPort == rhs.mPort;
}

bool MemCacheClient::Server::Set(
    const char *a_pszServer
)
{
    if (!a_pszServer || !*a_pszServer) { return false;  }

    size_t nLen = strlen(a_pszServer);
    if (nLen >= ADDRLEN) { return false;  }
    strcpy(mAddress, a_pszServer);

    mPort = 11211;
    char *pszPort = strchr(mAddress, ':');
    if (pszPort) {
        mPort    = atoi(pszPort + 1);
        *pszPort = 0;
    }

    mIp = inet_addr(mAddress);
    if (mIp == INADDR_NONE) { return false;  }

    struct in_addr addr;
    addr.s_addr = mIp;
    snprintf(mAddress, ADDRLEN, "%s:%d", inet_ntoa(addr), mPort);

    return true;
}

bool MemCacheClient::Server::Connect(
    int a_nTimeout
)
{
    // already connected? do nothing
    if (IsConnected()) {
        return true;
    }

    // only try to re-connect to a broken server occasionally
    time_t nNow;
#ifdef WIN32
    nNow = GetTickCount();
    if (nNow - mLastConnect < MEMCACHECLIENT_RECONNECT_SEC * 1000) { return false;  }
#else
    time(&nNow);
    if (nNow - mLastConnect < MEMCACHECLIENT_RECONNECT_SEC) { return false;  }
#endif
    mLastConnect = nNow;

    return ServerSocket::Connect(mIp, mPort, a_nTimeout);
}

///////////////////////////////////////////////////////////////////////////////
// MemCacheClient

MemCacheClient::MemCacheClient()
    : m_nTimeoutMs(1000)
{}

MemCacheClient::MemCacheClient(
    const MemCacheClient& rhs
)
{
    operator=(rhs);
}

MemCacheClient& MemCacheClient::operator=(
    const MemCacheClient& rhs
)
{
    m_nTimeoutMs = rhs.m_nTimeoutMs;
    ClearServers();
    m_rgpServer.resize(rhs.m_rgpServer.size());
    for (size_t n = 0; n < rhs.m_rgpServer.size(); ++n) {
        m_rgpServer[n] = new Server(*rhs.m_rgpServer[n]);
        if (!m_rgpServer[n]) { throw std::bad_alloc(); }
    }
    return *this;
}

MemCacheClient::~MemCacheClient()
{
    ClearServers();
}

void MemCacheClient::ClearServers()
{
    for (size_t n = 0; n < m_rgpServer.size(); ++n) {
        delete m_rgpServer[n];
    }
    m_rgpServer.clear();
}

bool MemCacheClient::AddServer(
    const char *a_pszServer
)
{
    // if we the server address is valid then we allow the server
    // to be added. All servers being added are assumed to be available
    // or to be soon made available. Uncontactable servers will cause
    // extra load on the database because the caching will not be available.
    Server *pServer = new Server;

    if (!pServer->Set(a_pszServer)) {
        delete pServer;
        return false;
    }
    for (size_t n = 0; n < m_rgpServer.size(); ++n) {
        if (*pServer == *m_rgpServer[n]) {
            return true;                              // already have it
        }
    }
    m_rgpServer.push_back(pServer);

    // for each salt we generate a string hash for the consistent hash
    // table. To ensure stability of the hashing for multiple servers,
    // we want to have a number of entries for each server.
    static const char *rgpSalt[] = {
        "{DD4C855D-7548-4804-8F1A-166CDBACEFE7}",
        "{9BF02198-1D29-4aa3-9466-A4AF4372D5B1}",
        "{0F20CD2F-ACF2-44bc-8CE3-54529D7B738D}",
        "{DEA60AAB-CFF9-4a20-A799-4E5E93369656}",
        "{C05167CC-57DA-40f2-9EB8-18F65E56FD21}",
        "{57939537-0966-49e7-B675-ACE63246BFA5}",
        "{F0C8BE5C-A0F1-478f-BC45-28D42AF0CA1E}"
    };

    string_t       sKey;
    ConsistentHash entry(0, pServer);
    for (size_t n = 0; n < sizeof(rgpSalt) / sizeof(rgpSalt[0]); ++n) {
        sKey        = pServer->GetAddress();
        sKey       += rgpSalt[n];
        entry.mHash = CreateKeyHash(sKey.data());
        m_rgServerHash.push_back(entry);
    }

    // sort the vector so that we can binary search it
    std::sort(m_rgServerHash.begin(), m_rgServerHash.end());

#if 0
    printf("\nSERVER RING (%d servers):\n", m_rgpServer.size());
    for (size_t n = 0; n < m_rgServerHash.size(); ++n) {
        printf("%08x = %s\n", m_rgServerHash[n].mHash,
               m_rgServerHash[n].mServer->GetAddress());
    }
#endif

    return true;
}

/*! Match a server by comparing pointers */
struct MemCacheClient::ConsistentHash::MatchServer
{
    /*! server being searched for */
    MemCacheClient::Server *mServer;

    /*! constructor
     *  @param aServer server to search for
     */
    MatchServer(MemCacheClient::Server *aServer)
        : mServer(aServer) { }

    /*! compare current entry against search entry
     *  @param rhs  entry to compare
     */
    bool operator()(const ConsistentHash& rhs) const { return rhs.mServer == mServer;  }
};

bool MemCacheClient::DelServer(
    const char *a_pszServer
)
{
    Server test;

    if (test.Set(a_pszServer)) {
        std::vector<Server *>::iterator i = m_rgpServer.begin();
        for ( ; i != m_rgpServer.end(); ++i) {
            Server *pServer = *i;
            if (test != *pServer) { continue; }

            delete pServer;
            m_rgpServer.erase(i);
            ConsistentHash::MatchServer server(pServer);
            m_rgServerHash.erase(
                std::partition(m_rgServerHash.begin(), m_rgServerHash.end(), server),
                m_rgServerHash.end());
            std::sort(m_rgServerHash.begin(), m_rgServerHash.end());
            return true;
        }
    }

    // not found
    return false;
}

void MemCacheClient::GetServers(
    std::vector<string_t>& a_rgServers
)
{
    a_rgServers.clear();
    a_rgServers.reserve(m_rgpServer.size());
    for (size_t n = 0; n < m_rgpServer.size(); ++n) {
        a_rgServers.push_back(m_rgpServer[n]->GetAddress());
    }
}

void MemCacheClient::SetTimeout(
    int a_nMilliseconds
)
{
    m_nTimeoutMs = a_nMilliseconds;
}

unsigned long MemCacheClient::CreateKeyHash(
    const char *a_pszKey
)
{
    const size_t LONG_COUNT = SHA1_DIGEST_LENGTH / sizeof(unsigned long);

    union {
        sha1_byte     as_char[SHA1_DIGEST_LENGTH];
        unsigned long as_long[LONG_COUNT];
    } output;

    assert(sizeof(output.as_char) == SHA1_DIGEST_LENGTH);
    assert(sizeof(output.as_long) == SHA1_DIGEST_LENGTH);

    SHA1(output.as_char, (const sha1_byte *)a_pszKey, (unsigned int)strlen(a_pszKey));
    return output.as_long[LONG_COUNT - 1];
}

MemCacheClient::Server *MemCacheClient::FindServer(
    const string_t& a_sKey
)
{
    // probably need some servers for this
    if (m_rgServerHash.empty()) {
        return NULL;
    }

    // find the next largest consistent hash value above this key hash
    ConsistentHash hash(CreateKeyHash(a_sKey.data()), NULL);
    std::vector<ConsistentHash>::iterator iServer =
        std::lower_bound(m_rgServerHash.begin(), m_rgServerHash.end(), hash);
    if (iServer == m_rgServerHash.end()) {
        iServer = m_rgServerHash.begin();
    }

    // ensure that this server is connected
    Server *pServer = iServer->mServer;
    if (!pServer->Connect(m_nTimeoutMs)) {
        return NULL;
    }
    return pServer;
}

/*! @brief Sort the requests into server order */
struct MemCacheClient::MemRequest::Sort
{
    /*! @brief Compare two requests for less ordering
     *  @param pl Request item
     *  @param pr Request item
     *  @return true if pl is less than pr.
     */
    bool operator()(const MemRequest *pl, const MemRequest *pr) const
    {
        return pl->mServer < pr->mServer;  // any order is fine
    }
};

int MemCacheClient::Combine(
    const char *a_pszType,
    MemRequest *a_rgItem,
    int        a_nCount
)
{
    if (a_nCount < 1) { return 0;  }

    MemRequest *rgpItem[MAX_REQUESTS] = { NULL };
    if (a_nCount > MAX_REQUESTS) {
        return -1;                          // invalid args
    }
    // initialize and find all of the servers for these items
    int nItemCount = 0;
    for (int n = 0; n < a_nCount; ++n) {
        // ensure that the key doesn't have a space in it
        assert(NULL == strchr(a_rgItem[n].mKey.data(), ' '));
        a_rgItem[n].mServer = FindServer(a_rgItem[n].mKey);
        if (a_rgItem[n].mServer) {
            rgpItem[nItemCount++] = &a_rgItem[n];
        }else {
            a_rgItem[n].mResult = MCERR_NOSERVER;
        }
    }
    if (nItemCount == 0) { return 0;  }

    // sort all requests into server order
    const static MemRequest::Sort sortOnServer = MemRequest::Sort();
    std::sort(&rgpItem[0], &rgpItem[nItemCount], sortOnServer);

    // send all requests
    char     szBuf[50];
    int      nItem = 0, nNext;
    string_t sRequest, sTemp;
    while (nItem < nItemCount) {
        for (nNext = nItem; nNext < nItemCount; ++nNext) {
            if (rgpItem[nItem]->mServer != rgpItem[nNext]->mServer) { break; }

            // create get request for all keys on this server
            if (*a_pszType == 'g') {
                if (nNext == nItem) { sRequest = "get"; }else{ sRequest.resize(sRequest.length() - 2); }
                sRequest += ' ';
                sRequest += rgpItem[nNext]->mKey;
                sRequest += "\r\n";
                rgpItem[nNext]->mResult = MCERR_NOTFOUND;
            }
            // create del request for all keys on this server
            else if (*a_pszType == 'd') {
                // delete <key> [<time>] [noreply]\r\n
                sRequest += "delete ";
                sRequest += rgpItem[nNext]->mKey;
                sRequest += ' ';
                snprintf(szBuf, sizeof(szBuf), "%ld", (long)rgpItem[nNext]->mExpiry);
                sRequest += szBuf;
                if (rgpItem[nNext]->mResult == MCERR_NOREPLY) {
                    sRequest += " noreply";
                }
                sRequest += "\r\n";
                if (rgpItem[nNext]->mResult != MCERR_NOREPLY) {
                    rgpItem[nNext]->mResult = MCERR_NOTFOUND;
                }
            }
        }

        // send the request. any socket error causes the server connection
        // to be dropped, so we return errors for all requests using that server.
        try {
            rgpItem[nItem]->mServer->SendBytes(
                sRequest.data(), sRequest.length());
        }catch (const ServerSocket::Exception&) {
            for (int n = nItem; n < nNext; ++n) {
                rgpItem[n]->mServer = NULL;
                rgpItem[n]->mResult = MCERR_NOSERVER;
            }
        }
        nItem = nNext;
    }

    // receive responses from all servers
    int nResponses = 0;
    for (nItem = 0; nItem < nItemCount; nItem = nNext) {
        // find the end of this server
        if (!rgpItem[nItem]->mServer) { nNext = nItem + 1; continue; }
        for (nNext = nItem + 1; nNext < nItemCount; ++nNext) {
            if (rgpItem[nItem]->mServer != rgpItem[nNext]->mServer) { break; }
        }

        // receive the responses. any socket error causes the server connection
        // to be dropped, so we return errors for all requests using that server.
        try {
            if (*a_pszType == 'g') {
                nResponses += HandleGetResponse(
                    rgpItem[nItem]->mServer,
                    &rgpItem[nItem], &rgpItem[nNext]);
            }else if (*a_pszType == 'd') {
                nResponses += HandleDelResponse(
                    rgpItem[nItem]->mServer,
                    &rgpItem[nItem], &rgpItem[nNext]);
            }
        }catch (const ServerSocket::Exception&) {
            rgpItem[nItem]->mServer->Disconnect();
            for (int n = nNext - 1; n >= nItem; --n) {
                if (rgpItem[nItem]->mServer != rgpItem[n]->mServer) { continue; }
                rgpItem[n]->mServer = NULL;
                rgpItem[n]->mResult = MCERR_NOSERVER;
            }
        }
    }

    return nResponses;
}

void MemCacheClient::ReceiveLine(
    Server    *a_pServer,
    string_t& a_sValue
)
{
    register char c;

    a_sValue = (c = a_pServer->GetByte());
    while (c != '\n') {
        a_sValue += (c = a_pServer->GetByte());
    }
}

int MemCacheClient::HandleGetResponse(
    Server     *a_pServer,
    MemRequest **a_ppBegin,
    MemRequest **a_ppEnd
)
{
    int nFound = 0;

    string_t sValue;

    for ( ; ; ) {
        // get the value
        ReceiveLine(a_pServer, sValue);
        if (sValue == "END\r\n") { break; }

        // if it isn't a value then we are in a bad state
        if (0 != strncmp(sValue.data(), "VALUE ", 6)) {
            throw ServerSocket::Exception("bad response");
        }

        // extract the key
        int n = (int)sValue.find(' ', 6);
        if (n < 1) { throw ServerSocket::Exception("bad response"); }
        string_t sKey(sValue, 6, n - 6);

        // extract the flags
        char     *pVal  = const_cast<char *>(sValue.data() + n + 1);
        unsigned nFlags = (unsigned)strtoul(pVal, &pVal, 10);
        if (*pVal++ != ' ') { throw ServerSocket::Exception("bad response"); }

        // extract the size
        unsigned nBytes = (unsigned)strtoul(pVal, &pVal, 10);
        if ((*pVal != ' ') && (*pVal != '\r')) { throw ServerSocket::Exception("bad response"); }

        // find this key in the array
        MemRequest *pItem = NULL;
        for (MemRequest **p = a_ppBegin; p < a_ppEnd; ++p) {
            if ((*p)->mKey == sKey) { pItem = *p; break; }
        }
        if (!pItem) {                            // key not found, discard the response
            a_pServer->DiscardBytes(nBytes + 2); // +2 == include final "\r\n"
            continue;
        }
        pItem->mFlags = nFlags;

        // extract the cas
        if (*pVal == ' ') {
            pItem->mCas = STRTOUL64(++pVal, &pVal, 10);
            if (*pVal != '\r') { throw ServerSocket::Exception("bad response"); }
        }

        // receive the data
        while (nBytes > 0) {
            char *pBuf     = pItem->mData.GetWriteBuffer(nBytes);
            int  nReceived = a_pServer->GetBytes(pBuf, nBytes);
            pItem->mData.CommitWriteBytes(nReceived);
            nBytes -= nReceived;
        }
        pItem->mResult = MCERR_OK;

        // discard the trailing "\r\n"
        if (('\r' != a_pServer->GetByte()) ||
            ('\n' != a_pServer->GetByte())) {
            throw ServerSocket::Exception("bad response");
        }

        ++nFound;
    }

    return nFound;
}

int MemCacheClient::HandleDelResponse(
    Server     *a_pServer,
    MemRequest **a_ppBegin,
    MemRequest **a_ppEnd
)
{
    string_t sValue;
    int      nResponses = 0;

    for (MemRequest **p = a_ppBegin; p < a_ppEnd; ++p) {
        MemRequest *pItem = *p;

        // no response for this entry
        if (pItem->mResult == MCERR_NOREPLY) { continue; }

        // get the value
        ReceiveLine(a_pServer, sValue);

        // success
        if (sValue == "DELETED\r\n") {
            pItem->mResult = MCERR_OK;
            ++nResponses;
            continue;
        }

        // the item with this key was not found
        if (sValue == "NOT_FOUND\r\n") {
            pItem->mResult = MCERR_NOTFOUND;
            ++nResponses;
            continue;
        }

        a_pServer->Disconnect();
        throw ServerSocket::Exception("bad response");
    }

    return nResponses;
}

MCResult MemCacheClient::IncDec(
    const char *a_pszType,
    const char *a_pszKey,
    uint64_t   *a_pnNewValue,
    uint64_t   a_nDiff,
    bool       a_bWantReply
)
{
    Server *pServer = FindServer(a_pszKey);

    if (!pServer) { return MCERR_NOSERVER;  }

    char     szBuf[50];
    string_t sRequest(a_pszType);
    sRequest += ' ';
    sRequest += a_pszKey;
    snprintf(szBuf, sizeof(szBuf), " " SPRINTF_UINT64, a_nDiff);
    sRequest += szBuf;
    if (!a_bWantReply) {
        sRequest += " noreply";
    }
    sRequest += "\r\n";

    try {
        pServer->SendBytes(sRequest.data(), sRequest.length());

        if (!a_bWantReply) {
            return MCERR_NOREPLY;
        }

        string_t sValue;
        sValue = pServer->GetByte();
        while (sValue[sValue.length() - 1] != '\n') {
            sValue += pServer->GetByte();
        }

        if (sValue == "NOT_FOUND\r\n") {
            return MCERR_NOTFOUND;
        }

        if (a_pnNewValue) {
            *a_pnNewValue = STRTOUL64(sValue.data(), NULL, 10);
        }
        return MCERR_OK;
    }catch (const ServerSocket::Exception&) {
        pServer->Disconnect();
        return MCERR_NOSERVER;
    }
}

int MemCacheClient::Store(
    const char *a_pszType,
    MemRequest *a_rgItem,
    int        a_nCount
)
{
    if (a_nCount < 1) { return 0;  }

    // initialize and find all of the servers for these items
    for (int n = 0; n < a_nCount; ++n) {
        // ensure that the key doesn't have a space in it
        assert(NULL == strchr(a_rgItem[n].mKey.data(), ' '));
        a_rgItem[n].mServer = FindServer(a_rgItem[n].mKey);

        if (!a_rgItem[n].mServer) {
            a_rgItem[n].mResult = MCERR_NOSERVER;
        }
    }

    char     szBuf[50];
    int      nResponses = 0;
    string_t sRequest;
    for (int n = 0; n < a_nCount; ++n) {
        if (!a_rgItem[n].mServer) { continue; }

        // <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n
        sRequest  = a_pszType;
        sRequest += ' ';
        sRequest += a_rgItem[n].mKey;
        snprintf(szBuf, sizeof(szBuf), " %u %ld %u",
                 a_rgItem[n].mFlags, (long)a_rgItem[n].mExpiry,
                 a_rgItem[n].mData.GetReadSize());
        sRequest += szBuf;
        if (*a_pszType == 'c') { // cas
            snprintf(szBuf, sizeof(szBuf), " " SPRINTF_UINT64, a_rgItem[n].mCas);
            sRequest += szBuf;
        }
        if (a_rgItem[n].mResult == MCERR_NOREPLY) {
            sRequest += " noreply";
        }
        sRequest += "\r\n";
        // send the request. any socket error causes the server connection
        // to be dropped, so we return errors for all requests using that server.
        try {
            a_rgItem[n].mServer->SendBytes(
                sRequest.data(), sRequest.length());
            a_rgItem[n].mServer->SendBytes(
                a_rgItem[n].mData.GetReadBuffer(),
                a_rgItem[n].mData.GetReadSize());
            a_rgItem[n].mServer->SendBytes("\r\n", 2);

            // done with these read bytes
            a_rgItem[n].mData.CommitReadBytes(
                a_rgItem[n].mData.GetReadSize());

            // if no reply is required then move on to the next request
            if (a_rgItem[n].mResult == MCERR_NOREPLY) {
                continue;
            }

            // handle this response
            HandleStoreResponse(a_rgItem[n].mServer, a_rgItem[n]);
            ++nResponses;
        }catch (const ServerSocket::Exception&) {
            for (int i = a_nCount - 1; i >= n; --i) {
                if (a_rgItem[n].mServer != a_rgItem[i].mServer) { continue; }
                a_rgItem[i].mServer = NULL;
                a_rgItem[i].mResult = MCERR_NOSERVER;
            }
            continue;
        }
    }

    return nResponses;
}

void MemCacheClient::HandleStoreResponse(
    Server      *a_pServer,
    MemRequest& a_oItem
)
{
    // get the value
    string_t sValue;

    ReceiveLine(a_pServer, sValue);

    // success
    if (sValue == "STORED\r\n") {
        a_oItem.mResult = MCERR_OK;
        return;
    }

    // data was not stored, but not because of an error.
    // This normally means that either that the condition for
    // an "add" or a "replace" command wasn't met, or that the
    // item is in a delete queue.
    if (sValue == "NOT_STORED\r\n") {
        a_oItem.mResult = MCERR_NOTSTORED;
        return;
    }

    // unknown response, connection may be bad
    a_pServer->Disconnect();
    throw ServerSocket::Exception("bad response");
}

int MemCacheClient::FlushAll(
    const char *a_pszServer,
    int        a_nExpiry
)
{
    char szRequest[50];

    snprintf(szRequest, sizeof(szRequest),
             "flush_all %u\r\n", a_nExpiry);

    Server test;
    if (a_pszServer && !test.Set(a_pszServer)) { return false;  }

    int nSuccess = 0;
    for (size_t n = 0; n < m_rgpServer.size(); ++n) {
        Server *pServer = m_rgpServer[n];
        if (a_pszServer && (*pServer != test)) { continue; }

        // ensure that we are connected
        if (!pServer->Connect(m_nTimeoutMs)) {
            continue;
        }

        try {
            // request
            pServer->SendBytes(szRequest, strlen(szRequest));

            // response
            string_t sValue;
            sValue = pServer->GetByte();
            while (sValue[sValue.length() - 1] != '\n') {
                sValue += pServer->GetByte();
            }
            if (sValue == "OK\r\n") {
                // done
                ++nSuccess;
            }else {
                // unknown response, connection may be bad
                pServer->Disconnect();
            }
        }catch (const ServerSocket::Exception&) {
            // data error
        }
    }

    return nSuccess;
}
