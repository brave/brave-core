// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef URL_GURL_H_
#define URL_GURL_H_

#include <optional>
#include <string>
#include <vector>

class GURL {
 public:
  GURL() = default;

  explicit GURL(const std::string& url) {
    Parse(url);
  }

  bool is_valid() const { return valid_; }

  bool SchemeIs(const std::string& scheme) const {
    return !scheme_.empty() && scheme_ == scheme;
  }

  bool has_username() const { return !username_.empty(); }
  bool has_password() const { return !password_.empty(); }
  bool has_port() const { return !port_.empty(); }
  bool has_query() const { return !query_.empty(); }
  bool has_ref() const { return !ref_.empty(); }

  bool HostIsIPAddress() const {
    if (host_.empty()) return false;
    // Check if host looks like an IPv4 address.
    int a, b, c, d;
    char dummy;
    if (std::sscanf(host_.c_str(), "%d.%d.%d.%d%c", &a, &b, &c, &d, &dummy) == 4) {
      return (a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 &&
              d >= 0 && d <= 255);
    }
    // Check if host looks like an IPv6 address (contains ':').
    if (host_.find(':') != std::string::npos) {
      return true;
    }
    return false;
  }

  std::string host() const { return host_; }

 private:
  void Parse(const std::string& url) {
    valid_ = !url.empty();

    // Find scheme.
    size_t colon = url.find("://");
    if (colon != std::string::npos) {
      scheme_ = url.substr(0, colon);
      size_t pos = colon + 3;

      // Find end of authority.
      size_t path_start = url.find('/', pos);
      size_t query_start = url.find('?', pos);
      size_t ref_start = url.find('#', pos);

      size_t authority_end = url.size();
      if (path_start != std::string::npos) authority_end = std::min(authority_end, path_start);
      if (query_start != std::string::npos) authority_end = std::min(authority_end, query_start);
      if (ref_start != std::string::npos) authority_end = std::min(authority_end, ref_start);

      std::string authority = url.substr(pos, authority_end - pos);

      // Parse userinfo.
      size_t at = authority.find('@');
      if (at != std::string::npos) {
        std::string userinfo = authority.substr(0, at);
        size_t colon_user = userinfo.find(':');
        if (colon_user != std::string::npos) {
          username_ = userinfo.substr(0, colon_user);
          password_ = userinfo.substr(colon_user + 1);
        } else {
          username_ = userinfo;
        }
        authority = authority.substr(at + 1);
      }

      // Parse host and port.
      size_t colon_port = authority.find(':');
      if (colon_port != std::string::npos) {
        host_ = authority.substr(0, colon_port);
        port_ = authority.substr(colon_port + 1);
      } else {
        host_ = authority;
      }

      // Parse path.
      if (path_start != std::string::npos) {
        size_t path_end = url.size();
        if (query_start != std::string::npos) path_end = std::min(path_end, query_start);
        if (ref_start != std::string::npos) path_end = std::min(path_end, ref_start);
        path_ = url.substr(path_start, path_end - path_start);
      }

      // Parse query.
      if (query_start != std::string::npos) {
        size_t query_end = url.size();
        if (ref_start != std::string::npos) query_end = std::min(query_end, ref_start);
        query_ = url.substr(query_start, query_end - query_start);
      }

      // Parse ref/fragment.
      if (ref_start != std::string::npos) {
        ref_ = url.substr(ref_start);
      }
    } else {
      valid_ = false;
    }
  }

  bool valid_ = false;
  std::string scheme_;
  std::string username_;
  std::string password_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string query_;
  std::string ref_;
};

#endif  // URL_GURL_H_
