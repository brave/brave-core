/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_SERVER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_SERVER_UTIL_H_

#include <string>

#include "base/no_destructor.h"

namespace brave_adaptive_captcha {

class ServerUtil {
 public:
  ServerUtil(const ServerUtil&) = delete;
  ServerUtil& operator=(const ServerUtil&) = delete;
  ~ServerUtil();

  static ServerUtil* GetInstance();

  std::string GetServerUrl(const std::string& path);
  void SetServerHostForTesting(const std::string& host);

 private:
  friend class base::NoDestructor<ServerUtil>;
  ServerUtil();

  std::string GetHost();

  std::string server_host_;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_SERVER_UTIL_H_
