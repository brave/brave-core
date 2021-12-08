/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/host_port_pair.h"

#include <string>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace net {
namespace {

bool HasAuthentication(const GURL& url) {
  return url.has_username() || url.has_password();
}

bool HasAuthentication(base::StringPiece str) {
  std::vector<base::StringPiece> auth_host = base::SplitStringPiece(
      str, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  return auth_host.size() == 2;
}

HostPortPair FromStringWithAuthentication(base::StringPiece str) {
  std::vector<base::StringPiece> auth_host = base::SplitStringPiece(
      str, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  HostPortPair host_port_pair =
      HostPortPair::FromString(std::string(auth_host[1]));

  std::vector<base::StringPiece> user_pass = base::SplitStringPiece(
      str, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  host_port_pair.set_username(std::string(user_pass[0]));
  host_port_pair.set_password(std::string(user_pass[1]));
  return host_port_pair;
}

std::string MaybeAddUsernameAndPassword(const HostPortPair* host_port_pair,
                                        const std::string& str) {
  std::string ret;
  if (!host_port_pair->username().empty()) {
    ret += host_port_pair->username();
    if (!host_port_pair->password().empty()) {
      ret += ':';
      ret += host_port_pair->password();
    }
    ret += '@';
  }
  return ret + str;
}

}  // namespace
}  // namespace net

#define BRAVE_HOST_PORT_PAIR_TO_STRING_ \
  ret = MaybeAddUsernameAndPassword(this, ret);

#define BRAVE_HOST_PORT_PAIR_FROM_STRING_ \
  if (HasAuthentication(str)) \
    return FromStringWithAuthentication(str);

#define BRAVE_HOST_PORT_PAIR_FROM_URL_ \
  if (HasAuthentication(url)) \
    return HostPortPair(url.username(), \
                        url.password(), \
                        url.HostNoBrackets(), \
                        static_cast<uint16_t>(url.EffectiveIntPort()));

#include "src/net/base/host_port_pair.cc"

#undef BRAVE_HOST_PORT_PAIR_TO_STRING_
#undef BRAVE_HOST_PORT_PAIR_FROM_STRING_
#undef BRAVE_HOST_PORT_PAIR_FROM_URL_

namespace net {

HostPortPair::~HostPortPair() = default;
HostPortPair::HostPortPair(const HostPortPair& host_port) = default;

HostPortPair::HostPortPair(base::StringPiece username,
                           base::StringPiece password,
                           base::StringPiece in_host,
                           uint16_t in_port)
    : username_(username),
      password_(password),
      host_(in_host),
      port_(in_port) {}

const std::string& HostPortPair::username() const {
  return username_;
}

const std::string& HostPortPair::password() const {
  return password_;
}

void HostPortPair::set_username(const std::string& in_username) {
    username_ = in_username;
}

void HostPortPair::set_password(const std::string& in_password) {
  password_ = in_password;
}

bool HostPortPair::operator<(const HostPortPair& other) const {
  return std::tie(port_, host_, username_, password_) <
      std::tie(other.port_, other.host_, other.username_, other.password_);
}

bool HostPortPair::Equals(const HostPortPair& other) const {
  return username_ == other.username_ && password_ == other.password_ &&
      host_ == other.host_ && port_ == other.port_;
}

}  // namespace net
