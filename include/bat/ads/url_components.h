/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_URL_COMPONENTS_H_
#define BAT_ADS_URL_COMPONENTS_H_

#include <string>

#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT UrlComponents {
  UrlComponents();
  explicit UrlComponents(const UrlComponents& components);
  ~UrlComponents();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string url;
  std::string scheme;
  std::string user;
  std::string hostname;
  std::string port;
  std::string query;
  std::string fragment;
};

}  // namespace ads

#endif  // BAT_ADS_URL_COMPONENTS_H_
