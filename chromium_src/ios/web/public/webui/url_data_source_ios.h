/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_

#include <cstdint>

namespace network::mojom {
enum class CSPDirectiveName : std::int32_t;
}  // namespace network::mojom

#define GetContentSecurityPolicyObjectSrc                 \
  GetContentSecurityPolicyObjectSrc_ChromiumImpl() const; \
  virtual std::string GetContentSecurityPolicyObjectSrc

#define ShouldServiceRequest                                    \
  ShouldServiceRequest_ChromiumImpl(const GURL& url) const;     \
  virtual bool ShouldServiceRequest(const GURL& url) const;     \
  virtual bool ShouldAddContentSecurityPolicy() const;          \
  virtual std::string GetContentSecurityPolicyFrameSrc() const; \
  virtual std::string GetContentSecurityPolicy(                 \
      network::mojom::CSPDirectiveName directive) const;        \
                                                                \
 private:                                                       \
  bool Dummy

#import "src/ios/web/public/webui/url_data_source_ios.h"  // IWYU pragma: export

#undef GetContentSecurityPolicyObjectSrc
#undef ShouldServiceRequest

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_URL_DATA_SOURCE_IOS_H_
