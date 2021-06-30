/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_wayback_machine_delegate_impl.h"

#include "base/command_line.h"
#include "brave/common/brave_switches.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"

// static
void BraveWaybackMachineDelegateImpl::AttachTabHelperIfNeeded(
    content::WebContents* web_contents) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBraveWaybackMachineExtension)) {
    BraveWaybackMachineTabHelper::CreateForWebContents(web_contents);
    auto* tab_helper =
        BraveWaybackMachineTabHelper::FromWebContents(web_contents);
    tab_helper->set_delegate(
        std::make_unique<BraveWaybackMachineDelegateImpl>());
  }
}

BraveWaybackMachineDelegateImpl::BraveWaybackMachineDelegateImpl() = default;
BraveWaybackMachineDelegateImpl::~BraveWaybackMachineDelegateImpl() = default;


void BraveWaybackMachineDelegateImpl::CreateInfoBar(
    content::WebContents* web_contents) {
  infobars::ContentInfoBarManager::FromWebContents(web_contents)
      ->AddInfoBar(CreateInfoBarView(
                       std::make_unique<BraveWaybackMachineInfoBarDelegate>(),
                       web_contents),
                   true);
}
