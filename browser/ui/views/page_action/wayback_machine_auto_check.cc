// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/wayback_machine_auto_check.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/infobars/wayback_machine_infobar_delegate.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/features.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

namespace {

BraveWaybackMachineTabHelper* GetTabHelper(tabs::TabInterface& tab) {
  content::WebContents* contents = tab.GetContents();
  return contents ? BraveWaybackMachineTabHelper::FromWebContents(contents)
                  : nullptr;
}

}  // namespace

WaybackMachineAutoCheck::WaybackMachineAutoCheck(tabs::TabInterface& tab)
    : tab_(tab) {}

WaybackMachineAutoCheck::~WaybackMachineAutoCheck() = default;

void WaybackMachineAutoCheck::OnWaybackStateChanged(WaybackState state) {
  switch (state) {
    case WaybackState::kNeedToCheck:
      if (ShouldCheck()) {
        if (auto* tab_helper = GetTabHelper(*tab_)) {
          is_checking_ = true;
          tab_helper->FetchSnapshotInfo();
        }
      }
      break;
    case WaybackState::kFetching:
      break;
    case WaybackState::kFound:
      if (std::exchange(is_checking_, false)) {
        MaybeShowInfoBar();
      }
      break;
    case WaybackState::kInitial:
    case WaybackState::kNotAvailable:
    case WaybackState::kLoaded:
      Reset();
      break;
  }
}

void WaybackMachineAutoCheck::Reset() {
  is_checking_ = false;
  if (content::WebContents* contents = tab_->GetContents()) {
    WaybackMachineInfoBarDelegate::Remove(contents);
  }
}

bool WaybackMachineAutoCheck::ShouldCheck() {
  if (!base::FeatureList::IsEnabled(
          brave_wayback_machine::features::kWaybackMachineAutoCheck)) {
    return false;
  }
  if (!tab_->IsActivated()) {
    return false;
  }
  content::WebContents* contents = tab_->GetContents();
  if (!contents) {
    return false;
  }
  auto* prefs = user_prefs::UserPrefs::Get(contents->GetBrowserContext());
  return prefs->GetBoolean(kBraveWaybackMachineAutoCheckEnabled);
}

void WaybackMachineAutoCheck::MaybeShowInfoBar() {
  if (!tab_->IsActivated()) {
    return;
  }
  auto* tab_helper = GetTabHelper(*tab_);
  if (!tab_helper || tab_helper->wayback_state() != WaybackState::kFound) {
    return;
  }
  WaybackMachineInfoBarDelegate::Create(tab_->GetContents(),
                                        *tab_helper->snapshot_info());
}
