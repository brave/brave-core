// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef NET_BASE_IP_ADDRESS_H_
#define NET_BASE_IP_ADDRESS_H_

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace net {

class IPAddress {
 public:
  IPAddress() = default;

  static IPAddress IPv4Localhost() {
    IPAddress addr;
    addr.type_ = Type::kIPv4;
    addr.ipv4_ = {127, 0, 0, 1};
    return addr;
  }

  static std::optional<IPAddress> FromIPLiteral(const std::string& literal) {
    IPAddress addr;
    if (addr.type_ == Type::kUnknown) {
      // Try IPv4
      struct in_addr sa4;
      if (inet_pton(AF_INET, literal.c_str(), &sa4) == 1) {
        addr.type_ = Type::kIPv4;
        addr.ipv4_ = {
          static_cast<uint8_t>(sa4.S_un.S_un_b.s_b1),
          static_cast<uint8_t>(sa4.S_un.S_un_b.s_b2),
          static_cast<uint8_t>(sa4.S_un.S_un_b.s_b3),
          static_cast<uint8_t>(sa4.S_un.S_un_b.s_b4)
        };
        return addr;
      }
      // Try IPv6
      struct in6_addr sa6;
      if (inet_pton(AF_INET6, literal.c_str(), &sa6) == 1) {
        addr.type_ = Type::kIPv6;
        std::memcpy(addr.ipv6_.data(), sa6.s6_bytes, 16);
        return addr;
      }
    }
    return std::nullopt;
  }

  bool IsValid() const { return type_ != Type::kUnknown; }

  bool IsPubliclyRoutable() const {
    return IsValid() && !IsLoopback() && !IsLinkLocal() && !IsUniqueLocalIPv6() && !IsPrivate();
  }

  bool IsLoopback() const {
    if (type_ == Type::kIPv4) {
      return ipv4_[0] == 127;
    }
    if (type_ == Type::kIPv6) {
      return (ipv6_[0] == 0x00 && ipv6_[1] == 0x00 && ipv6_[2] == 0x00 &&
              ipv6_[3] == 0x00 && ipv6_[4] == 0x00 && ipv6_[5] == 0x00 &&
              ipv6_[6] == 0x00 && ipv6_[7] == 0x00 && ipv6_[8] == 0x00 &&
              ipv6_[9] == 0x00 && ipv6_[10] == 0x00 && ipv6_[11] == 0x00 &&
              ipv6_[12] == 0x00 && ipv6_[13] == 0x00 && ipv6_[14] == 0x00 &&
              ipv6_[15] == 0x01);
    }
    return false;
  }

  bool IsLinkLocal() const {
    if (type_ == Type::kIPv4) {
      return (ipv4_[0] == 169 && ipv4_[1] == 254);
    }
    if (type_ == Type::kIPv6) {
      return (ipv6_[0] == 0xfe && ipv6_[1] == 0x80);
    }
    return false;
  }

  bool IsUniqueLocalIPv6() const {
    if (type_ == Type::kIPv6) {
      return (ipv6_[0] == 0xfc || ipv6_[0] == 0xfd);
    }
    return false;
  }

  bool IsPrivate() const {
    if (type_ == Type::kIPv4) {
      return (ipv4_[0] == 10) ||
             (ipv4_[0] == 172 && ipv4_[1] >= 16 && ipv4_[1] <= 31) ||
             (ipv4_[0] == 192 && ipv4_[1] == 168);
    }
    return false;
  }

 private:
  enum class Type { kUnknown, kIPv4, kIPv6 };
  Type type_ = Type::kUnknown;
  std::array<uint8_t, 4> ipv4_{};
  std::array<uint8_t, 16> ipv6_{};
};

}  // namespace net

#endif  // NET_BASE_IP_ADDRESS_H_
