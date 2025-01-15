/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WEBUI_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WEBUI_SOURCE_H_

#include <string>

#include "base/containers/span.h"
#include "build/build_config.h"

namespace content {
class WebContents;
class WebUI;
class WebUIDataSource;
}  // namespace content

namespace webui {
struct ResourcePath;
}  // namespace webui

// Add brave resource path mapping and localized strings to new data source.
content::WebUIDataSource* CreateAndAddWebUIDataSource(
    content::WebUI* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id,
    bool disable_trusted_types_csp = false);

#if !BUILDFLAG(IS_ANDROID)

// Provide html with background color so we can avoid flash of
// different colors as the page loads, especially for New Tab Pages.
void AddBackgroundColorToSource(content::WebUIDataSource* source,
                                content::WebContents* contents);

#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WEBUI_SOURCE_H_
