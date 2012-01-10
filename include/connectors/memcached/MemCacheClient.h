/*! @file       MemCacheClient.h
    @version    1.2
    @brief      Basic memcached client
 */
/*! @mainpage

    <table>
        <tr><th>Library     <td>MemCacheClient
        <tr><th>Author      <td>Brodie Thiesfield [code at jellycan dot com]
        <tr><th>Source      <td>http://code.jellycan.com/memcacheclient/
    </table>

    A basic C++ client for a memcached server. 
    
    This client was designed for use with Windows software but with some tweaking
    there is no reason that it shouldn't also build on posix systems.

    @section features FEATURES

    -   MIT Licence allows free use in all software (including GPL and commercial)
    -   multi-platform (Windows 95/98/ME/NT/2K/XP, Linux, Unix)
    -   multiple servers with consistent server hashing
    -   support for most memcached commands
    -   supports multiple requests in a single batch

    @section usage USAGE

    The following steps are required to use MemCacheClient. 

    <ol>

    <li> Include the MemCacheClient.*, ReadWriteBuffer.* and md5.* files in your project.

<pre>@#include "MemCacheClient.h"</pre>

    <li> Create an instance of the MemCacheClient object.

<pre>MemCacheClient * pClient = new MemCacheClient;</pre>

    <li> Add some servers to the client.

<pre>pClient->AddServer("127.0.0.1");</pre>

    <li> Make a request.

<pre>MemCacheClient::MemRequest req;
req.mKey  = "datakey";
req.mData.WriteBytes("foobar", 5);
pClient->Add(req);
if (req.mResult == MCERR_OK) ...</pre>

    </ol>

    @section notes NOTES

    See the protocol document for memcached at:
    http://code.sixapart.com/svn/memcached/trunk/server/doc/protocol.txt

    The included MD5 library is from the LUA project: http://www.keplerproject.org/md5/
    MD5 was designed and implemented by Roberto Ierusalimschy and Marcela Ozorio Suarez. 
    The implementation is not derived from licensed software. The DES 56 C library was 
    implemented by Stuart Levy and uses a MIT licence too. 


    @section licence MIT LICENCE

    The licence text below is the boilerplate "MIT Licence" used from:
    http://www.opensource.org/licenses/mit-license.php

    Copyright (c) 2008, Brodie Thiesfield

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is furnished
    to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef INCLUDED_MemCacheClient
#define INCLUDED_MemCacheClient

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef WIN32
# include <stdint.h>
#endif

#include <string>
#include <vector>

#include "ReadWriteBuffer.h"

// ----------------------------------------------------------------------------
/*! @brief Result code for requests to a server. 

    Success codes are greater or equal to 0. 
    Failure codes are less than 0. 
 */
enum MCResult { 
    // success: n >= 0
    MCERR_OK        = 0,    //!< Success
    MCERR_NOREPLY   = 1,    //!< Success assumed (no reply requested)
    MCERR_NOTSTORED = 2,    //!< Success but item not stored (see memcached docs)
    MCERR_NOTFOUND  = 3,    //!< Success but item not found (see memcached docs)

    // failure: n < 0
    MCERR_NOSERVER  = -1    //!< Failure, connection error with server
};

// ----------------------------------------------------------------------------
/*! @brief Public client interface for memcached. 

    Create an instance of this class, add all servers to it and then call 
    request methods. A single instance of the client is not threadsafe. 
    Use explicit locking or an instance per thread. Each instance will maintain
    a single TCP socket open to each server.
 */
class MemCacheClient 
{
    class Server;                           //!< server connection implementation

public:
    typedef std::string         string_t;   //!< Abstract the string class for internal use
#ifdef WIN32
    typedef unsigned __int64    uint64_t;   //!< 64-bit unsigned type
#else
    typedef unsigned long long  uint64_t;   //!< 64-bit unsigned type
#endif

    /*! @brief Input and output structure for most requests to the server */
    struct MemRequest 
    {
        struct Sort;        //!< sort requests into server order
        Server * mServer;   //!< server that will be used for this request

    public:
        /*! @brief constructor */
        MemRequest() { Clear(); }

        /*! @brief Clear the structure in preparation for a new request. */
        void Clear() { 
            mServer = NULL;
            mKey    = "";
            mFlags  = 0;
            mExpiry = 0; 
            mCas    = 0;
            mResult = MCERR_OK;
            mData.SetEmpty();
            mContext = NULL;
        }

    public:
        /*! @brief The request key. 
        
            Set this value to the key that should be used for the request. 
            It will not be modified by MemCacheClient. 
         */
        string_t mKey;

        /*! @brief Flags for the request. 
        
            These flags will be sent to the server when using one of the storage requests 
            and returned with a get request. The flags are not used by MemCacheClient. 
         */
        unsigned int mFlags;

        /*! @brief Expiry time for the data. 
        
            This is used only by storage requests. The expiry time is in seconds. 
            0 = no expiry. 1..2592000 (30 days) = expiry time relative to now. 
            All other values are an explicit time_t value. 
         */
        time_t mExpiry; 

        /*! @brief CAS value returned by Gets and sent by CheckSet */
        uint64_t mCas;

        /*! @brief Result of a request. 
            
            Set this to MCERR_NOREPLY prior to calling the function if a reply isn't 
            required. Any other value will request a reply from the server and 
            return the result. 
         */
        MCResult mResult; 

        /*! @brief Data to be sent to the server in a storage request, or data returned 
            from the server for get requests. 
         */
        ReadWriteBuffer mData;

        /*! @brief User managed context for this request. */
        void * mContext;
    };

public:
    /*! @brief Maximum number of request objects in a single call */
    const static int MAX_REQUESTS = 50;

    /*! @brief Initialise the memcached client */
    MemCacheClient();

    /*! @brief Initialise the memcached client with copy. */
    MemCacheClient(const MemCacheClient &); 

    /*! @brief Copy another client. 
        
        All existing servers will be disconnected and removed. New connections 
        will be created for all of the servers copied from the second client. 
     */
    MemCacheClient & operator=(const MemCacheClient &); 

    /*! @brief Destructor. 
        
        All servers will be disconnected. 
     */
    ~MemCacheClient();

    /*! @brief Set the network timeout for all operations. 
    
        All network operations share the same timeout.

        @param a_nTimeoutMs     Timeout in milliseconds
     */
    void SetTimeout(int a_nTimeoutMs);

    /*-----------------------------------------------------------------------*/
    /*! @{ @name Servers */

    /*! @brief Add a server to the client. 
    
        This function will fail only if the address is not a valid IP 
        address or PORT. It will not fail if the server cannot be contacted 
        but will instead continue to occasionally attempt connections 
        to that server. 

        @param a_pszServer  The server to be added specified as IP[:PORT]. 
                            The port will default to 11211 if not supplied.

        @return true if server was added
     */
    bool AddServer(const char * a_pszServer);

    /*! @brief Delete a server from the client. 
    
        The server should be specified as documented in AddServer. 

        @param a_pszServer  The server to be added specified as IP[:PORT]. 
                            The port will default to 11211 if not supplied.

        @return true if server was deleted
     */
    bool DelServer(const char * a_pszServer);

    /*! @brief Request the list of current servers 

        @param a_rgServers  list of all servers registered to this client
     */
    void GetServers(std::vector<string_t> & a_rgServers);

    /*! @brief Disconnect from and remove all servers */
    void ClearServers();

    /*-----------------------------------------------------------------------*/
    /*! @} */
    /*! @{ @name Single Requests */

    /*! @brief Add an item to the cache. 
    
        This will fail if the item already exists at the server.

        @param a_oItem  Item to be added
        @return Number of items with a success result.
     */
    inline int Add(MemRequest & a_oItem) { return Store("add", &a_oItem, 1); }

    /*! @brief Set an item to the cache. 
    
        This will always set the item at the server regardless of if it 
        already exists or not.

        @param a_oItem  Item to be updated
        @return Number of items with a success result.
     */
    inline int Set(MemRequest & a_oItem) { return Store("set", &a_oItem, 1); }

    /*! @brief Replace an item in the cache. 
    
        This will fail if the item does not already exist at the server.

        @param a_oItem  Item to be updated
        @return Number of items with a success result.
     */
    inline int Replace(MemRequest & a_oItem) { return Store("replace", &a_oItem, 1); }

    /*! @brief Append data to an existing item in the cache.

        @param a_oItem  Item to be updated
        @return Number of items with a success result.
     */
    inline int Append(MemRequest & a_oItem) { return Store("append", &a_oItem, 1); }

    /*! @brief Prepend data to an existing item in the cache.

        @param a_oItem  Item to be updated
        @return Number of items with a success result.
     */
    inline int Prepend(MemRequest & a_oItem) { return Store("prepend", &a_oItem, 1); }

    /*! @brief Set the data to an existing item in the cache only if it has not been
        modified since it was last retrieved. 
        
        This requires the mCas member to be set in the request. Use the Gets 
        command to retrieve an item with a valid mCas member.

        @param a_oItem  Item to be updated
        @return Number of items with a success result.
     */
    inline int CheckSet(MemRequest & a_oItem) { return Store("cas", &a_oItem, 1); }

    /*! @brief Get an item from the cache.

        @param a_oItem  Item to be retrieved
        @return Number of items with a success result.
     */
    inline int Get(MemRequest & a_oItem) { return Combine("get", &a_oItem, 1); }

    /*! @brief Get an item from the cache including the CAS data.

        @param a_oItem  Item to be retrieved
        @return Number of items with a success result.
     */
    inline int Gets(MemRequest & a_oItem) { return Combine("gets", &a_oItem, 1); }

    /*! @brief Delete an item from the cache.

        @param a_oItem  Item to be removed.
        @return Number of items with a success result.
     */
    inline int Del(MemRequest & a_oItem) { return Combine("del", &a_oItem, 1); }


    /*! @brief Increment a value at the server. 
    
        See the INCR command in the memcached protocol. 

        @param a_pszKey         Key to increment
        @param a_pnNewValue     Receive the new value if desired. If a_bWantReply is false, 
                                this value will not be updated. If a_bWantReply is true, a
                                reply will stil be requested from the server even if this
                                item is NULL.
        @param a_nDiff          Positive integer to increment the value at the server by.
        @param a_bWantReply     Should a reply be requested from the server.
        @return MCResult
     */
    inline MCResult Increment(const char * a_pszKey, uint64_t * a_pnNewValue = NULL, uint64_t a_nDiff = 1, bool a_bWantReply = true) { 
        return IncDec("incr", a_pszKey, a_pnNewValue, a_nDiff, a_bWantReply);
    }

    /*! @brief Decrement a value at the server. 
    
        See the DECR command in the memcached protocol. 

        @param a_pszKey         Key to decrement
        @param a_pnNewValue     Receive the new value if desired. If a_bWantReply is false, 
                                this value will not be updated. If a_bWantReply is true, a
                                reply will stil be requested from the server even if this
                                item is NULL.
        @param a_nDiff          Positive integer to decrement the value at the server by.
        @param a_bWantReply     Should a reply be requested from the server.
        @return MCResult
     */
    inline MCResult Decrement(const char * a_pszKey, uint64_t * a_pnNewValue = NULL, uint64_t a_nDiff = 1, bool a_bWantReply = true) { 
        return IncDec("decr", a_pszKey, a_pnNewValue, a_nDiff, a_bWantReply); 
    }

    /*-----------------------------------------------------------------------*/
    /*! @} */
    /*! @{ @name Multiple Requests */

    /*! @brief Add multiple items to the cache.
    
        This will fail if the item already exists at the server.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be added
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Add(MemRequest * a_rgItem, int a_nCount) { return Store("add", a_rgItem, a_nCount); }

    /*! @brief Set multiple items to the cache. 
    
        This will always set the item at the server regardless of if it already exists or not.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be added
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Set(MemRequest * a_rgItem, int a_nCount) { return Store("set", a_rgItem, a_nCount); }

    /*! @brief Replace multiple items in the cache. 
    
        This will fail if the item does not already exist at the server.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be added
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Replace(MemRequest * a_rgItem, int a_nCount) { return Store("replace", a_rgItem, a_nCount); }

    /*! @brief Append data to multiple existing items in the cache.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be added
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Append(MemRequest * a_rgItem, int a_nCount) { return Store("append", a_rgItem, a_nCount); }

    /*! @brief Prepend data to multiple existing items in the cache.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be added
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Prepend(MemRequest * a_rgItem, int a_nCount) { return Store("prepend", a_rgItem, a_nCount); }

    /*! @brief Set the data to multiple existing items in the cache only if it has not been
        modified since it was last retrieved. 
        
        This requires the mCas member to be set in the request. Use the Gets command to 
        retrieve an item with a valid mCas member.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be added
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int CheckSet(MemRequest * a_rgItem, int a_nCount) { return Store("cas", a_rgItem, a_nCount); }

    /*! @brief Get an item from the cache.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be retrieved
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Get(MemRequest * a_rgItem, int a_nCount) { return Combine("get", a_rgItem, a_nCount); }

    /*! @brief Get multiple items from the cache including the CAS data.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be retrieved
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Gets(MemRequest * a_rgItem, int a_nCount) { return Combine("gets", a_rgItem, a_nCount); }

    /*! @brief Delete multiple items from the cache.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_rgItem     Array of items to be retrieved
        @param a_nCount     Number of items in the array
        @return Number of items with a success result
     */
    inline int Del(MemRequest * a_rgItem, int a_nCount) { return Combine("del", a_rgItem, a_nCount); }

    /*! @brief Send a flush_all command to a specific server or all servers.
        
        @note   Each request is independent of other requests in the array. Failure 
                of one request does not imply failure of any other. 

        @param a_pszServer      Server to flush. If NULL all servers will be flushed.
        @param a_nExpiry        Expiry time for the flush. The server will wait before executing
                                the cache flush. See MemRequest::mExpiry for details of possible
                                values. 
        @return Number of servers that were flushed.
     */
    int FlushAll(const char * a_pszServer = NULL, int a_nExpiry = 0);

    /*! @} */

private:
    /*-----------------------------------------------------------------------*/
    /*! @{ 
        @internal 
        @name Internal 
     */

    std::vector<Server*> m_rgpServer;       //!< current servers

    int m_nTimeoutMs;                       //!< network timeout in millisec for all operations

    /*! @brief Maintain the N:1 hash key to server relationship used for consistent hashing. 
        
        Consistent hashing for the servers ensures that even with changes to the server list, 
        many of the data keys will continue to mapped to the same server. This ensures that 
        changes to the server list doesn't invalidate the entire cache.
     */
    struct ConsistentHash { 
        unsigned long   mHash;      //!< hash value
        Server *        mServer;    //!< server implementation

        /*! constructor */
        ConsistentHash(unsigned long aHash, Server * aServer) 
            : mHash(aHash), mServer(aServer) { }
        /*! copy constructor */
        ConsistentHash(const ConsistentHash & rhs) { operator=(rhs); }
        /*! copy */
        ConsistentHash & operator=(const ConsistentHash & rhs) {
            mHash = rhs.mHash; mServer = rhs.mServer; return *this; }
        /*! comparison on hash value */
        bool operator<(const ConsistentHash & rhs) const { return mHash < rhs.mHash; }
        /*! equality on hash value */
        bool operator==(const ConsistentHash & rhs) const { return mHash == rhs.mHash; }
        /*! match on server pointer */
        struct MatchServer;
    };

    /*! @brief Consistent hash ring for servers.
    
        This vector is in sorted order on the hash key. 
     */
    std::vector<ConsistentHash> m_rgServerHash;

    /*! create a hash value from the key */
    unsigned long CreateKeyHash(const char * a_pszKey);

    /*! find which server will be used to handle the key */
    Server * FindServer(const string_t & a_sKey);

    /*! send storage requests (add, set, cas, replace, append, prepend) */
    int Store(const char * a_pszType, MemRequest * a_rgItem, int a_nCount);

    /*! handle a single storage response from a server */
    void HandleStoreResponse(Server * a_pServer, MemRequest & a_oItem);
    
    /*! send multiple commands to all servers for commands that can be combined (get, gets, del) */
    int Combine(const char * a_pszType, MemRequest * a_rgItem, int a_nCount);

    /*! receive a line of text ending with a \n character */
    void ReceiveLine(Server * a_pServer, string_t & a_sValue);

    /*! handle a single get or gets response from a server */
    int HandleGetResponse(Server * a_pServer, MemRequest ** a_ppBegin, MemRequest ** a_ppEnd);

    /*! handle a single del response from a server */
    int HandleDelResponse(Server * a_pServer, MemRequest ** a_ppBegin, MemRequest ** a_ppEnd);

    /*! send an incr or decr request to a server and handle the response */
    MCResult IncDec(const char * a_pszType, const char * a_pszKey, uint64_t * a_pnNewValue, uint64_t a_nDiff, bool a_bWantReply);

    /*! @} */
};

#endif // INCLUDED_MemCacheClient
