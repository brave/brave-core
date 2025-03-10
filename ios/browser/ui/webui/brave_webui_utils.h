// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEBUI_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEBUI_SOURCE_H_

#include <string>

#include "base/containers/span.h"
#include "build/build_config.h"

namespace web {
class WebUIIOS;
class WebUIIOSDataSource;
}  // namespace web

namespace webui {
struct ResourcePath;
}  // namespace webui

namespace brave {

// Add brave resource path mapping and localized strings to new data source.
web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_paths,
    bool disable_trusted_types_csp = false);

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEBUI_SOURCE_H_
