/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_

#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/webui/url_data_manager_ios.h"
#include "ios/web/webui/url_data_source_ios_impl.h"

/// Override that allows BraveWebUIIOSDataSource to access
/// WebUIIOSDataSourceImpl members. Also, allows it to construct a
/// WebUIIOSDataSourceImpl with its own source. Avoids duplicating a lot of
/// code.

class URLDataSourceIOS;
class BraveWebUIIOSDataSource;

#define WebUIIOSDataSourceTest                                  \
  WebUIIOSDataSourceTest;                                       \
  friend class ::BraveWebUIIOSDataSource;                       \
  explicit WebUIIOSDataSourceImpl(std::string_view source_name, \
                                  URLDataSourceIOS* source)

#include <ios/web/webui/web_ui_ios_data_source_impl.h>  // IWYU pragma: export

#undef WebUIIOSDataSourceTest

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_WEB_UI_IOS_DATA_SOURCE_IMPL_H_
