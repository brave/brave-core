/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_DATA_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_DATA_SOURCE_H_

#include <cstdint>

namespace network::mojom {
enum class CSPDirectiveName : std::int32_t;
}  // namespace network::mojom

#define DisableDenyXFrameOptions                                 \
  DisableDenyXFrameOptions() = 0;                                \
  virtual void OverrideContentSecurityPolicy(                    \
      network::mojom::CSPDirectiveName directive,                \
      const std::string& value) = 0;                             \
  virtual void AddFrameAncestor(const GURL& frame_ancestor) = 0; \
  virtual void DisableTrustedTypesCSP

#include "src/ios/web/public/webui/web_ui_ios_data_source.h"  // IWYU pragma: export

#undef DisableDenyXFrameOptions

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_WEBUI_WEB_UI_IOS_DATA_SOURCE_H_
