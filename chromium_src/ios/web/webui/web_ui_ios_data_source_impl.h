/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_

#include <cstdint>

namespace network::mojom {
enum class CSPDirectiveName : std::int32_t;
}  // namespace network::mojom

#define should_replace_i18n_in_js_                                          \
  should_replace_i18n_in_js_;                                               \
                                                                            \
 public:                                                                    \
  void OverrideContentSecurityPolicy(                                       \
      network::mojom::CSPDirectiveName directive, const std::string& value) \
      override;                                                             \
  void AddFrameAncestor(const GURL& frame_ancestor) override;               \
  void DisableTrustedTypesCSP() override;                                   \
                                                                            \
 private:                                                                   \
  base::flat_map<network::mojom::CSPDirectiveName, std::string>             \
      csp_overrides_;                                                       \
  std::set<GURL> frame_ancestors_

#include "src/ios/web/webui/web_ui_ios_data_source_impl.h"  // IWYU pragma: export

#undef should_replace_i18n_in_js_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_
