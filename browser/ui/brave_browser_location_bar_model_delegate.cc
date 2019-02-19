/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_location_bar_model_delegate.h"

#if !defined(OS_ANDROID)
#include "components/omnibox/browser/vector_icons.h"  // nogncheck
#endif                                                // !defined(OS_ANDROID)

#include "content/public/common/url_constants.h"
const gfx::VectorIcon*
BraveBrowserLocationBarModelDelegate::GetVectorIconOverride() const {
#if !defined(OS_ANDROID)
  GURL url;
  GetURL(&url);

  if (url.SchemeIs(content::kBraveUIScheme))
    return &omnibox::kProductIcon;

  return BrowserLocationBarModelDelegate::GetVectorIconOverride();
#endif

  return nullptr;
}
