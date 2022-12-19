/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_SERVICE_DELEGATE_IMPL_H_

#include "brave/components/sidebar/sidebar_service_delegate.h"
#include "components/prefs/pref_member.h"

class PrefService;

class SidebarServiceDelegateImpl : public sidebar::SidebarServiceDelegate {
 public:
  explicit SidebarServiceDelegateImpl(PrefService* prefs);
  ~SidebarServiceDelegateImpl() override;

  // sidebar::SidebarServiceDelegate:
  void MoveSidebarToRightTemporarily() override;
  void RestoreSidebarAlignmentIfNeeded() override;

 private:
  void OnSidebarAlignmentChanged();

  raw_ptr<PrefService> prefs_;

  bool changing_sidebar_alignment_temporarily_ = false;

  BooleanPrefMember sidebar_alignment_;
};

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_SERVICE_DELEGATE_IMPL_H_
