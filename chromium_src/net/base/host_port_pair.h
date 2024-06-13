/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_HOST_PORT_PAIR_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_HOST_PORT_PAIR_H_

#include <compare>
#include <string>
#include <string_view>

#define HostPortPair HostPortPair_ChromiumImpl

#include "src/net/base/host_port_pair.h"  // IWYU pragma: export

#undef HostPortPair

namespace net {

class NET_EXPORT HostPortPair : public HostPortPair_ChromiumImpl {
 public:
  HostPortPair();
  HostPortPair(std::string_view in_host, uint16_t in_port);
  HostPortPair(std::string_view username,
               std::string_view password,
               std::string_view in_host,
               uint16_t in_port);
  HostPortPair(const HostPortPair&);
  ~HostPortPair();

  static HostPortPair FromURL(const GURL& url);
  static HostPortPair FromSchemeHostPort(
      const url::SchemeHostPort& scheme_host_port);
  static HostPortPair FromIPEndPoint(const IPEndPoint& ipe);
  static HostPortPair FromString(std::string_view str);
  static std::optional<HostPortPair> FromValue(const base::Value& value);

  const std::string& username() const { return username_; }
  const std::string& password() const { return password_; }

  void set_username(const std::string& username);
  void set_password(const std::string& password);

  std::strong_ordering operator<=>(const HostPortPair& other) const;
  bool operator==(const HostPortPair& other) const;
  bool Equals(const HostPortPair& other) const;

  std::string ToString() const;

 private:
  // NOLINTNEXTLINE(runtime/explicit)
  HostPortPair(const HostPortPair_ChromiumImpl& other);

  std::string username_;
  std::string password_;
};

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_HOST_PORT_PAIR_H_
