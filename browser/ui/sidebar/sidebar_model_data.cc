/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_model_data.h"

#include <string>

#include "brave/browser/ui/sidebar/sidebar_web_contents_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/tab_helper.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/mojom/view_type.mojom.h"
#endif

namespace sidebar {

namespace {

void AttachTabHelpersForSidebar(content::WebContents* contents) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::SetViewType(contents, extensions::mojom::ViewType::kTabContents);
  extensions::TabHelper::CreateForWebContents(contents);
#endif
}

}  // namespace

SidebarModelData::SidebarModelData(Profile* profile) : profile_(profile) {}

SidebarModelData::~SidebarModelData() {
  // Make sure to destroy contents first because contents refers delegate.
  contents_.reset();
  contents_delegate_.reset();
}

content::WebContents* SidebarModelData::GetWebContents() {
  if (!contents_) {
    content::WebContents::CreateParams params(profile_);
    contents_ = content::WebContents::Create(params);
    contents_delegate_.reset(new SidebarWebContentsDelegate);
    contents_->SetDelegate(contents_delegate_.get());

    AttachTabHelpersForSidebar(contents_.get());
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
