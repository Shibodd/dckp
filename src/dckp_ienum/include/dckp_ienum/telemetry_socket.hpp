#pragma once

#include <cstring>
#include <sstream>
#include <vector>

#include <arpa/inet.h>
#include <unistd.h>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <cereal/archives/json.hpp>

#define TEL_SEND(x) TelemetrySocket::get().send(#x, x)

#define TEL_PERROR(os, x)                                                                          \
    ((os) << x << " failed: " << strerrorname_np(errno) << " (" << strerror(errno) << ')')
#define TEL_PERROR_SPRINT(x) TEL_PERROR(std::ostringstream(), x).str()

#define TEL_SPRINT(x) [&]() { return (std::ostringstream() << x).str(); }()
#define TEL_ASSERT_PERROR(cond, x)                                                                 \
    do {                                                                                           \
        if (not(cond))                                                                             \
            throw std::runtime_error(TEL_PERROR_SPRINT(x));                                        \
    } while (false)
#define TEL_ASSERT(cond, exception, x)                                                             \
    do {                                                                                           \
        if (not(cond))                                                                             \
            throw exception(TEL_SPRINT(x));                                                        \
    } while (false)

struct TelemetrySocket
{
    template<typename Type>
    void send(const std::string& name, Type& type) const
    {
        static thread_local std::vector<char> buffer;
        buffer.clear();

        {
            boost::iostreams::back_insert_device sink(buffer);
            boost::iostreams::stream os(sink);
            cereal::JSONOutputArchive ar(os);
            ar(cereal::make_nvp(name, type));
        }
        ssize_t sz = static_cast<ssize_t>(buffer.size());
        TEL_ASSERT_PERROR(sz == sendto(mSock,
                                       buffer.data(),
                                       buffer.size(),
                                       0,
                                       reinterpret_cast<const sockaddr*>(&mDestination),
                                       sizeof(mDestination)),
                          "sendto");
    }

    static const TelemetrySocket& get()
    {
        static TelemetrySocket value("127.0.0.1", 9870);
        return value;
    }

    TelemetrySocket(const char* hostname, uint16_t port)
    {
        mSock = socket(AF_INET, SOCK_DGRAM, 0);
        TEL_ASSERT_PERROR(mSock > 0, "socket");

        memset(&mDestination, 0, sizeof(mDestination));
        TEL_ASSERT_PERROR(inet_pton(AF_INET, hostname, &mDestination.sin_addr) > 0, "inet_pton");
        mDestination.sin_family = AF_INET;
        mDestination.sin_port = htons(port);
    }

    ~TelemetrySocket() { close(mSock); }

  private:
    int mSock;
    sockaddr_in mDestination;
};