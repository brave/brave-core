// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/web_applications/app_browser_controller.h"

#include "brave/browser/ui/brave_scheme_utils.h"

#define FormatUrlOrigin FormatUrlOrigin_ChromiumImpl
#include "src/chrome/browser/ui/web_applications/app_browser_controller.cc"
#undef FormatUrlOrigin

namespace web_app {

// static
std::u16string AppBrowserController::FormatUrlOrigin(
    const GURL& url,
    url_formatter::FormatUrlTypes format_types) {
  std::u16string url_string = FormatUrlOrigin_ChromiumImpl(url, format_types);
  brave_utils::ReplaceChromeToBraveScheme(url_string);

  return url_string;
}

}  // namespace web_app
