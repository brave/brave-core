/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_wayback_machine_delegate_impl.h"

#include <memory>

#include "base/command_line.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

// static
void BraveWaybackMachineDelegateImpl::AttachTabHelperIfNeeded(
    content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBraveWaybackMachineExtension) &&
      !brave::IsTorProfile(context)) {
    BraveWaybackMachineTabHelper::CreateForWebContents(web_contents);
    auto* tab_helper =
        BraveWaybackMachineTabHelper::FromWebContents(web_contents);
    tab_helper->SetInfoBarManager(
        InfoBarService::FromWebContents(web_contents));
    tab_helper->set_delegate(std::make_unique<BraveWaybackMachineDelegateImpl>(
        Profile::FromBrowserContext(context)));
  }
}

BraveWaybackMachineDelegateImpl::BraveWaybackMachineDelegateImpl(
    Profile* profile)
    : profile_(profile) {
}

BraveWaybackMachineDelegateImpl::~BraveWaybackMachineDelegateImpl() = default;

bool BraveWaybackMachineDelegateImpl::IsWaybackMachineEnabled() const {
  return profile_->GetPrefs()->GetBoolean(kBraveWaybackMachineEnabled);
}
