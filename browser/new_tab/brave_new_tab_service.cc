/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/brave_new_tab_service.h"

#include "chrome/common/webui_url_constants.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"

BraveNewTabService::BraveNewTabService(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  PreloadNewTab();
}

BraveNewTabService::~BraveNewTabService() {}

std::unique_ptr<content::WebContents> BraveNewTabService::GetNewTabContent() {
  auto preloaded = std::move(cached_new_tab_);
  PreloadNewTab();
  return preloaded;
}

void BraveNewTabService::PreloadNewTab() {
  cached_new_tab_ = content::WebContents::Create(
      content::WebContents::CreateParams(browser_context_));
  cached_new_tab_->GetController().LoadURL(
      GURL(chrome::kChromeUINewTabURL), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
}

void BraveNewTabService::Reset() {
  cached_new_tab_.reset();
}
