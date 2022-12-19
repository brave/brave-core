/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_service_delegate_impl.h"

#include "base/auto_reset.h"
#include "brave/components/sidebar/pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

SidebarServiceDelegateImpl::SidebarServiceDelegateImpl(PrefService* prefs)
    : prefs_(prefs) {
  sidebar_alignment_.Init(
      prefs::kSidePanelHorizontalAlignment, prefs_,
      base::BindRepeating(
          &SidebarServiceDelegateImpl::OnSidebarAlignmentChanged,
          base::Unretained(this)));
}

SidebarServiceDelegateImpl::~SidebarServiceDelegateImpl() = default;

void SidebarServiceDelegateImpl::MoveSidebarToRightTemporarily() {
  // If this value was changed by users, we should respect that.
  if (!prefs_->FindPreference(prefs::kSidePanelHorizontalAlignment)
           ->IsDefaultValue())
    return;

  base::AutoReset<bool> resetter(&changing_sidebar_alignment_temporarily_,
                                 true);
  prefs_->SetBoolean(prefs::kSidePanelHorizontalAlignment,
                     /* align right = */ true);
}

void SidebarServiceDelegateImpl::RestoreSidebarAlignmentIfNeeded() {
  if (!prefs_->GetBoolean(sidebar::kSidebarAlignmentChangedTemporarily))
    return;

  base::AutoReset<bool> resetter(&changing_sidebar_alignment_temporarily_,
                                 true);
  prefs_->ClearPref(prefs::kSidePanelHorizontalAlignment);
}

void SidebarServiceDelegateImpl::OnSidebarAlignmentChanged() {
  prefs_->SetBoolean(
      sidebar::kSidebarAlignmentChangedTemporarily,
      changing_sidebar_alignment_temporarily_ && *sidebar_alignment_);
}
