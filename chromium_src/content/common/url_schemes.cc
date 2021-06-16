/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "url/url_util.h"

#include "content/common/content_export.h"

namespace {
const char kIPFSScheme[] = "ipfs";
const char kIPNSScheme[] = "ipns";
}  // namespace
namespace content {
CONTENT_EXPORT void RegisterContentSchemes();
}

#define RegisterContentSchemes RegisterContentSchemes_ChromiumImpl
#include "../../../../content/common/url_schemes.cc"
#undef RegisterContentSchemes

namespace content {
void RegisterContentSchemes() {
  if (!g_registered_url_schemes) {
    url::AddStandardScheme(kIPFSScheme, url::SCHEME_WITH_HOST);
    url::AddStandardScheme(kIPNSScheme, url::SCHEME_WITH_HOST);
  }
  RegisterContentSchemes_ChromiumImpl();
}
}  // namespace content
#undef RegisterContentSchemes
