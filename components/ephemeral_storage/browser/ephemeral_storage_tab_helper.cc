/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/browser/ephemeral_storage_tab_helper.h"

#include <string>

#include "base/feature_list.h"
#include "components/content_settings/core/common/features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

using content::BrowserContext;
using content::NavigationHandle;
using content::WebContents;

namespace ephemeral_storage {

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() {}

EphemeralStorageTabHelper::EphemeralStorageTabHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

void EphemeralStorageTabHelper::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;
  if (navigation_handle->IsSameDocument())
    return;

  ClearEphemeralStorage();
}

void EphemeralStorageTabHelper::WebContentsDestroyed() {
  ClearEphemeralStorage();
}

void EphemeralStorageTabHelper::ClearEphemeralStorage() {
  if (!base::FeatureList::IsEnabled(blink::kBraveEphemeralStorage)) {
    return;
  }

  WebContents* contents = web_contents();
  contents->GetBrowserContext()->ClearEphemeralStorageForHost(
      contents->GetRenderViewHost(), contents->GetSiteInstance());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper)

}  // namespace ephemeral_storage
