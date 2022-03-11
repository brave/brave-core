/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_web_contents_delegate.h"

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "chrome/browser/ui/browser_finder.h"

namespace sidebar {

namespace {

SidebarController* GetSidebarController(content::WebContents* source) {
  auto* brave_browser =
      static_cast<BraveBrowser*>(chrome::FindBrowserWithWebContents(source));
  if (!brave_browser)
    return nullptr;

  return brave_browser->sidebar_controller();
}

}  // namespace

SidebarWebContentsDelegate::SidebarWebContentsDelegate() = default;
SidebarWebContentsDelegate::~SidebarWebContentsDelegate() = default;

bool SidebarWebContentsDelegate::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (auto* sidebar_controller = GetSidebarController(source))
    return sidebar_controller->sidebar()->HandleKeyboardEvent(source, event);

  return false;
}

}  // namespace sidebar
