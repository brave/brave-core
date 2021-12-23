/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_drm_tab_helper.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "brave/browser/widevine/constants.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
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
                   kWidevineComponentId) != component_ids.end();
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

BraveDrmTabHelper::BraveDrmTabHelper(content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<BraveDrmTabHelper>(*contents),
      brave_drm_receivers_(contents, this) {
  auto* updater = g_browser_process->component_updater();
  // We don't need to observe if widevine is already registered.
  if (!IsAlreadyRegistered(updater))
    observer_.Observe(updater);
}

BraveDrmTabHelper::~BraveDrmTabHelper() {}

// static
void BraveDrmTabHelper::BindBraveDRM(
    mojo::PendingAssociatedReceiver<brave_drm::mojom::BraveDRM> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;

  auto* tab_helper = BraveDrmTabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return;
  tab_helper->brave_drm_receivers_.Bind(rfh, std::move(receiver));
}

bool BraveDrmTabHelper::ShouldShowWidevineOptIn() const {
  // If the user already opted in, don't offer it.
  content::WebContents& web_contents =
      const_cast<content::WebContents&>(GetWebContents());
  PrefService* prefs =
      static_cast<Profile*>(web_contents.GetBrowserContext())->GetPrefs();
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
    RequestWidevinePermission(&GetWebContents(), false /* for_restart */);
  }
}

void BraveDrmTabHelper::OnEvent(Events event, const std::string& id) {
  if (event == ComponentUpdateService::Observer::Events::COMPONENT_UPDATED &&
      id == kWidevineComponentId) {
#if defined(OS_LINUX)
    // Ask restart instead of reloading. Widevine is only usable after
    // restarting on linux. This restart permission request is only shown if
    // this tab asks widevine explicitely.
    if (is_widevine_requested_)
      RequestWidevinePermission(&GetWebContents(), true /* for_restart*/);
#else
    // When widevine is ready to use, only active tab that requests widevine is
    // reloaded automatically.
    if (is_widevine_requested_)
      ReloadIfActive(&GetWebContents());
#endif
    // Stop observing component update event.
    observer_.Reset();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveDrmTabHelper);
