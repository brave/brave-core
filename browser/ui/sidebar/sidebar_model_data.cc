/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_model_data.h"

#include <string>

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace sidebar {

SidebarModelData::SidebarModelData(Profile* profile) : profile_(profile) {}

SidebarModelData::~SidebarModelData() = default;

content::WebContents* SidebarModelData::GetWebContents() {
  if (!contents_) {
    content::WebContents::CreateParams params(profile_);
    contents_ = content::WebContents::Create(params);
  }

  return contents_.get();
}

void SidebarModelData::LoadURL(const GURL& url) {
  GetWebContents()->GetController().LoadURL(url, content::Referrer(),
                                            ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                            std::string());
}

bool SidebarModelData::IsLoaded() const {
  if (!contents_)
    return false;

  return !contents_->GetVisibleURL().is_empty();
}

}  // namespace sidebar
