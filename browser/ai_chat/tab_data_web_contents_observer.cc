// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tab_data_web_contents_observer.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "components/tab_collections/public/tab_interface.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace ai_chat {

namespace {

mojom::TabDataPtr CreateTabDataFromWebContents(
    content::WebContents* web_contents) {
  auto tab = mojom::TabData::New();
  tab->content_id =
      web_contents->GetController().GetLastCommittedEntry()->GetUniqueID();
  tab->title = base::UTF16ToUTF8(web_contents->GetTitle());
  tab->url = web_contents->GetLastCommittedURL();
  return tab;
}

}  // namespace

TabDataWebContentsObserver::TabDataWebContentsObserver(
    int32_t tab_handle,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      tab_handle_(tab_handle),
      service_(*TabTrackerServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext())) {}

TabDataWebContentsObserver::~TabDataWebContentsObserver() {
  service_->UpdateTab(tab_handle_, nullptr);
}

void TabDataWebContentsObserver::TitleWasSet(content::NavigationEntry* entry) {
  UpdateTab();
}

void TabDataWebContentsObserver::PrimaryPageChanged(content::Page& page) {
  UpdateTab();
}

void TabDataWebContentsObserver::UpdateTab() {
  auto tab = CreateTabDataFromWebContents(web_contents());
  tab->id = tab_handle_;
  service_->UpdateTab(tab_handle_, std::move(tab));
}

}  // namespace ai_chat
