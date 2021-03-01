/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_drm_tab_helper.h"

#include <algorithm>
#include <vector>

#include "brave/browser/widevine/widevine_utils.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"

using component_updater::ComponentUpdateService;

namespace {
bool IsAlreadyRegistered(ComponentUpdateService* cus) {
  std::vector<std::string> component_ids;
  component_ids = cus->GetComponentIDs();
  return std::find(component_ids.begin(), component_ids.end(),
                   BraveDrmTabHelper::kWidevineComponentId) !=
         component_ids.end();
}
#if !defined(OS_LINUX)
content::WebContents* GetActiveWebContents() {
  if (Browser* browser = chrome::FindLastActive())
    return browser->tab_strip_model()->GetActiveWebContents();
  return nullptr;
}

void ReloadIfActive(content::WebContents* web_contents) {
  if (GetActiveWebContents() == web_contents)
    web_contents->GetController().Reload(content::ReloadType::NORMAL, false);
}
#endif

}  // namespace

// static
const char BraveDrmTabHelper::kWidevineComponentId[] =
    "oimompecagnajdejgnnjijobebaeigek";

BraveDrmTabHelper::BraveDrmTabHelper(content::WebContents* contents)
    : WebContentsObserver(contents),
      receivers_(contents, this),
      observer_(this) {
  auto* updater = g_browser_process->component_updater();
  // We don't need to observe if widevine is already registered.
  if (!IsAlreadyRegistered(updater))
    observer_.Add(updater);
}

BraveDrmTabHelper::~BraveDrmTabHelper() {}

bool BraveDrmTabHelper::ShouldShowWidevineOptIn() const {
  // If the user already opted in, don't offer it.
  PrefService* prefs =
      static_cast<Profile*>(web_contents()->GetBrowserContext())->GetPrefs();
  if (IsWidevineOptedIn() || !prefs->GetBoolean(kAskWidevineInstall)) {
    return false;
  }

  return is_widevine_requested_;
}

void BraveDrmTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  is_widevine_requested_ = false;
  is_permission_requested_ = false;
}

void BraveDrmTabHelper::OnWidevineKeySystemAccessRequest() {
  is_widevine_requested_ = true;

  if (ShouldShowWidevineOptIn() && !is_permission_requested_) {
    is_permission_requested_ = true;
    RequestWidevinePermission(web_contents(), false /* for_restart */);
  }
}

void BraveDrmTabHelper::OnEvent(Events event, const std::string& id) {
  if (event == ComponentUpdateService::Observer::Events::COMPONENT_UPDATED &&
      id == kWidevineComponentId) {
#if defined(OS_LINUX)
    // Ask restart instead of reloading. Widevine is only usable after
    // restarting on linux. This restart permission request is only shown if
    // this tab asks widevine explicitly.
    if (is_widevine_requested_)
      RequestWidevinePermission(web_contents(), true /* for_restart*/);
#else
    // When widevine is ready to use, only active tab that requests widevine is
    // reloaded automatically.
    if (is_widevine_requested_)
      ReloadIfActive(web_contents());
#endif
    // Stop observing component update event.
    observer_.RemoveAll();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveDrmTabHelper)
