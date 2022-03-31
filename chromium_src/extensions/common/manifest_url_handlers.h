/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_URL_HANDLERS_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_URL_HANDLERS_H_

#define GetHomepageURL                               \
  GetHomepageURL_Unused(const Extension* extension); \
  static const GURL GetHomepageURL

#define GetWebStoreURL                               \
  GetWebStoreURL_Unused(const Extension* extension); \
  static const GURL GetWebStoreURL

#include "src/extensions/common/manifest_url_handlers.h"

#undef GetHomepageURL
#undef GetWebStoreURL

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_URL_HANDLERS_H_
