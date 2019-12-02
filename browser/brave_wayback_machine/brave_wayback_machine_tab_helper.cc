/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wayback_machine/brave_wayback_machine_tab_helper.h"

#include "base/command_line.h"
#include "brave/browser/brave_wayback_machine/brave_wayback_machine_util.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_switches.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"

// static
void BraveWaybackMachineTabHelper::AttachTabHelperIfNeeded(
    content::WebContents* contents) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBraveWaybackMachineExtension))
    return;

  if (brave::IsTorProfile(contents->GetBrowserContext()))
    return;

  BraveWaybackMachineTabHelper::CreateForWebContents(contents);
}

BraveWaybackMachineTabHelper::BraveWaybackMachineTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents) {
}

void BraveWaybackMachineTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!IsWaybackMachineEnabled(web_contents()->GetBrowserContext()) ||
      !navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (const net::HttpResponseHeaders* header =
          navigation_handle->GetResponseHeaders()) {
    CheckWaybackMachineIfNeeded(web_contents(), header->response_code());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWaybackMachineTabHelper)
