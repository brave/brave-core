/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVER_SERVER_HOST_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVER_SERVER_HOST_BASE_H_

#include <string>

namespace ads {

class ServerHostBase {
 public:
  ServerHostBase();
  virtual ~ServerHostBase();

  virtual std::string Get() const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVER_SERVER_HOST_BASE_H_
