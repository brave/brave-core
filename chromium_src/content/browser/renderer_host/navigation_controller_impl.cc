/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace {

// We want to rewrite brave -> chrome, but without setting the virtual url to
// brave
void MaybeRewriteVirtualURL(GURL* virtual_url) {
  if (virtual_url && virtual_url->SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    *virtual_url = virtual_url->ReplaceComponents(replacements);
  }
}

}  // namespace

#include <content/browser/renderer_host/navigation_controller_impl.cc>
