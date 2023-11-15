/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "content/public/common/url_constants.h"

#if !BUILDFLAG(IS_ANDROID)
#include "components/omnibox/browser/vector_icons.h"
#endif

BraveLocationBarModelDelegate::BraveLocationBarModelDelegate(Browser* browser)
    : BrowserLocationBarModelDelegate(browser) {}

BraveLocationBarModelDelegate::~BraveLocationBarModelDelegate() = default;

#if !BUILDFLAG(IS_ANDROID)
const gfx::VectorIcon* BraveLocationBarModelDelegate::GetVectorIconOverride() const {
  GURL url;
  GetURL(&url);
  if (url.SchemeIs(content::kBraveUIScheme)) {
    return &omnibox::kProductIcon;
  }
  return nullptr;
}
#endif
