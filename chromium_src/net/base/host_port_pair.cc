// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../../../../net/base/host_port_pair.cc"

#include "net/base/host_port_pair.h"

#include "base/strings/string_number_conversions.h"

namespace net {

HostPortPair::~HostPortPair() = default;
HostPortPair::HostPortPair(const HostPortPair& host_port) = default;

HostPortPair::HostPortPair(const std::string& username,
                           const std::string& password,
                           const std::string& in_host, uint16_t in_port)
    : username_(username), password_(password),
      host_(in_host), port_(in_port) {
}

std::string HostPortPair::ToString() const {
  std::string ret;
  if (username_.size() != 0 || password_.size() != 0) {
    ret += username_;
    if (password_.size() != 0) {
      ret += ':';
      ret += password_;
    }
    ret += '@';
  }
  ret += HostForURL();
  ret += ':';
  ret += base::NumberToString(port_);
  return ret;
}

}  // namespace net
