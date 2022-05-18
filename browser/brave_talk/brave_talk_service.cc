/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_talk/brave_talk_service.h"

#include <algorithm>

#include "base/callback.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_talk {

BraveTalkService::BraveTalkService() = default;
BraveTalkService::~BraveTalkService() {
  StopObserving();
  Shutdown();
}

void BraveTalkService::GetDeviceID(
    content::WebContents* contents,
    base::OnceCallback<void(const std::string&)> callback) {
  if (on_received_device_id_)
    std::move(on_received_device_id_).Run("");

  on_received_device_id_ = std::move(callback);
  StartObserving(contents);
}

void BraveTalkService::StartObserving(content::WebContents* contents) {
  if (web_contents())
    StopObserving();

  Observe(contents);
}

void BraveTalkService::StopObserving() {
  Observe(nullptr);
}

void BraveTalkService::DidStartNavigation(content::NavigationHandle* handle) {
  if (!handle->IsInMainFrame())
    return;

  // On any navigation of the main frame stop observing the web contents.
  StopObserving();
}

void BraveTalkService::ShareTab(content::WebContents* target_contents) {
  if (!web_contents())
    return;
  auto* registry =
      BraveTalkTabCaptureRegistry::Get(target_contents->GetBrowserContext());

  content::DesktopMediaID media_id(
      content::DesktopMediaID::TYPE_WEB_CONTENTS,
      content::DesktopMediaID::kNullId,
      content::WebContentsMediaCaptureId(
          target_contents->GetMainFrame()->GetProcess()->GetID(),
          target_contents->GetMainFrame()->GetRoutingID()));
  std::string device_id =
      registry->AddRequest(target_contents, media_id, web_contents());
  if (on_received_device_id_)
    std::move(on_received_device_id_).Run(device_id);
}

}  // namespace brave_talk