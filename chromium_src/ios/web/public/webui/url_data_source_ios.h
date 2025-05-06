/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_

#define GetContentSecurityPolicyObjectSrc                   \
  GetContentSecurityPolicyObjectSrc() const;                \
  virtual std::string GetContentSecurityPolicyBase() const; \
  virtual std::string GetContentSecurityPolicyFrameSrc

#import "src/ios/web/public/webui/url_data_source_ios.h"  // IWYU pragma: export

#undef GetContentSecurityPolicyObjectSrc

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_
