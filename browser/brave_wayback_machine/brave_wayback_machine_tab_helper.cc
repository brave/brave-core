/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wayback_machine/brave_wayback_machine_tab_helper.h"

#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"

namespace {

bool ShouldUseWaybackMachine(int response) {
  static base::flat_set<int> responses =
      { 404, 408, 410, 451, 500, 502, 503, 504,
        509, 520, 521, 523, 524, 525, 526 };
  return responses.find(response) != responses.end();
}

bool IsWaybackMachinePrefsEnabled(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->GetPrefs()->GetBoolean(kBraveWaybackMachineEnabled);
}

}  // namespace

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
  if (!IsWaybackMachinePrefsEnabled(web_contents()->GetBrowserContext()) ||
      !navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  const net::HttpResponseHeaders* header =
      navigation_handle->GetResponseHeaders();
  if (header && ShouldUseWaybackMachine(header->response_code())) {
    // Do Wayback!
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWaybackMachineTabHelper)
