// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tab_informer.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/tab_informer_service_factory.h"
#include "brave/components/ai_chat/core/browser/tab_informer_service.h"
#include "brave/components/ai_chat/core/common/mojom/tab_informer.mojom.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"

namespace ai_chat {

namespace {

mojom::TabPtr FromWebContents(content::WebContents* web_contents) {
  auto tab = mojom::Tab::New();
  tab->content_id =
      web_contents->GetController().GetVisibleEntry()->GetUniqueID();
  tab->title = base::UTF16ToUTF8(web_contents->GetTitle());
  tab->url = web_contents->GetLastCommittedURL();
  return tab;
}

}  // namespace

TabInformer::TabInformer(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      tab_id_(
          tabs::TabInterface::GetFromContents(web_contents)->GetTabHandle()),
      service_(TabInformerServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext())) {}

TabInformer::~TabInformer() {
  if (service_) {
    service_->UpdateTab(tab_id_, nullptr);
  }
}

void TabInformer::TitleWasSet(content::NavigationEntry* entry) {
  UpdateTab();
}

void TabInformer::PrimaryPageChanged(content::Page& page) {
  UpdateTab();
}

void TabInformer::UpdateTab() {
  if (!service_) {
    return;
  }

  auto tab = FromWebContents(web_contents());
  tab->id = tab_id_;
  service_->UpdateTab(tab_id_, std::move(tab));
}

}  // namespace ai_chat
