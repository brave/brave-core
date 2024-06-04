/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_

// #include "services/network/public/mojom/content_security_policy.mojom.h"

#define ShouldServiceRequest                                    \
  ShouldServiceRequest(const GURL& url) const;                  \
  virtual bool ShouldAddContentSecurityPolicy() const;          \
  virtual std::string GetContentSecurityPolicyFrameSrc() const; \
                                                                \
 private:                                                       \
  bool Dummy

#import "src/ios/web/public/webui/url_data_source_ios.h"  // IWYU pragma: export

#undef ShouldServiceRequest

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_
