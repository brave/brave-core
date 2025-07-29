// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tab_ui_helper.h"

#include <string>

#include "brave/browser/ui/tabs/features.h"

#define GetTitle GetTitle_ChromiumImpl

#include <chrome/browser/ui/tab_ui_helper.cc>

#undef GetTitle

void TabUIHelper::SetCustomTitle(const std::u16string& title) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs));

  if (title == custom_title_) {
    return;
  }

  if (title.empty()) {
    custom_title_.reset();
    return;
  }

  custom_title_ = title;
}

std::u16string TabUIHelper::GetTitle() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs)) {
    return GetTitle_ChromiumImpl();
  }

  return custom_title_.value_or(GetTitle_ChromiumImpl());
}

void TabUIHelper::UpdateLastOrigin() {
  // If the origin has changed since the last time we got the title, reset the
  // custom title. This is to ensure that the custom title is not stale.
  const auto origin =
      url::Origin::Create(web_contents()->GetLastCommittedURL());
  if ((last_origin_.opaque() && origin.opaque()) ||
      last_origin_.IsSameOriginWith(origin)) {
    return;
  }

  custom_title_.reset();
  last_origin_ = origin;
}
