/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/host_port_pair.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace net {
namespace {

bool HasAuthentication(const GURL& url) {
  return url.has_username() || url.has_password();
}

bool HasAuthentication(std::string_view str) {
  std::vector<std::string_view> auth_host = base::SplitStringPiece(
      str, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  return auth_host.size() == 2;
}

HostPortPair FromStringWithAuthentication(std::string_view str) {
  std::vector<std::string_view> auth_host = base::SplitStringPiece(
      str, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  HostPortPair host_port_pair =
      HostPortPair::FromString(std::string(auth_host[1]));

  std::vector<std::string_view> user_pass = base::SplitStringPiece(
      auth_host[0], ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

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

#define HostPortPair HostPortPair_ChromiumImpl

#include "src/net/base/host_port_pair.cc"

#undef HostPortPair

namespace net {

HostPortPair::HostPortPair() = default;
HostPortPair::~HostPortPair() = default;
HostPortPair::HostPortPair(const HostPortPair& host_port) = default;

// private
HostPortPair::HostPortPair(const HostPortPair_ChromiumImpl& other)
    : HostPortPair_ChromiumImpl(other.host(), other.port()) {}

HostPortPair::HostPortPair(std::string_view in_host, uint16_t in_port)
    : HostPortPair_ChromiumImpl(in_host, in_port) {}

HostPortPair::HostPortPair(std::string_view username,
                           std::string_view password,
                           std::string_view in_host,
                           uint16_t in_port)
    : HostPortPair_ChromiumImpl(in_host, in_port) {
  set_username(std::string(username));
  set_password(std::string(password));
}

// static
HostPortPair HostPortPair::FromURL(const GURL& url) {
  if (HasAuthentication(url)) {
    return HostPortPair(url.username(), url.password(), url.HostNoBrackets(),
                        static_cast<uint16_t>(url.EffectiveIntPort()));
  }
  return HostPortPair_ChromiumImpl::FromURL(url);
}

// static
HostPortPair HostPortPair::FromSchemeHostPort(
    const url::SchemeHostPort& scheme_host_port) {
  return HostPortPair_ChromiumImpl::FromSchemeHostPort(scheme_host_port);
}

// static
HostPortPair HostPortPair::FromIPEndPoint(const IPEndPoint& ipe) {
  return HostPortPair_ChromiumImpl::FromIPEndPoint(ipe);
}

// static
HostPortPair HostPortPair::FromString(std::string_view str) {
  if (HasAuthentication(str)) {
    return FromStringWithAuthentication(str);
  }
  return HostPortPair_ChromiumImpl::FromString(str);
}

// static
std::optional<HostPortPair> HostPortPair::FromValue(const base::Value& value) {
  auto r = HostPortPair_ChromiumImpl::FromValue(value);
  if (r) {
    return HostPortPair(r.value());
  }
  return std::nullopt;
}

void HostPortPair::set_username(const std::string& in_username) {
  username_ = in_username;
}

void HostPortPair::set_password(const std::string& in_password) {
  password_ = in_password;
}

std::strong_ordering HostPortPair::operator<=>(
    const HostPortPair& other) const {
  if (port() != other.port()) {
    // note: port() is r-value, can't be used with std::tie
    return port() <=> other.port();
  }

  auto tie = [](const HostPortPair& v) {
    return std::tie(v.host(), v.username(), v.password());
  };
  return tie(*this) <=> tie(other);
}

bool HostPortPair::operator==(const HostPortPair& other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

bool HostPortPair::Equals(const HostPortPair& other) const {
  return *this == other;
}

std::string HostPortPair::ToString() const {
  const std::string& ret = HostPortPair_ChromiumImpl::ToString();
  return MaybeAddUsernameAndPassword(this, ret);
}

}  // namespace net
