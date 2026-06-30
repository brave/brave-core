// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_controller_delegate_impl.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace brave_news {

BraveNewsControllerDelegateImpl::BraveNewsControllerDelegateImpl(
    Profile* profile)
    : profile_(profile) {
  CHECK(profile_);
}

BraveNewsControllerDelegateImpl::~BraveNewsControllerDelegateImpl() = default;

void BraveNewsControllerDelegateImpl::OpenSettings() {
  // The New Tab Page shows the Brave News customize modal when opened with this
  // query param (see the `openSettings` handling in the New Tab Page).
  NavigateParams params(
      profile_,
      GURL(base::StrCat({kBraveUINewTabURL, "?openSettings=BraveNews"})),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
}

}  // namespace brave_news
