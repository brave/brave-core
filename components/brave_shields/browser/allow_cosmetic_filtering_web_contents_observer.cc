/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/allow_cosmetic_filtering_web_contents_observer.h"

#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace brave_shields {

AllowCosmeticFilteringWebContentsObserver::
    AllowCosmeticFilteringWebContentsObserver(
        content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<AllowCosmeticFilteringWebContentsObserver>(
          *web_contents) {}

AllowCosmeticFilteringWebContentsObserver::
    ~AllowCosmeticFilteringWebContentsObserver() {
  brave_shields_remotes_.clear();
}

void AllowCosmeticFilteringWebContentsObserver::RenderFrameCreated(
    content::RenderFrameHost* rfh) {
  if (rfh)
    GetBraveShieldsRemote(rfh)->AllowCosmeticFiltering();
}

void AllowCosmeticFilteringWebContentsObserver::RenderFrameDeleted(
    content::RenderFrameHost* rfh) {
  brave_shields_remotes_.erase(rfh);
}

void AllowCosmeticFilteringWebContentsObserver::RenderFrameHostChanged(
    content::RenderFrameHost* old_rfh,
    content::RenderFrameHost* new_rfh) {
  if (old_rfh)
    RenderFrameDeleted(old_rfh);

  if (new_rfh)
    RenderFrameCreated(new_rfh);
}

mojo::AssociatedRemote<brave_shields::mojom::BraveShields>&
AllowCosmeticFilteringWebContentsObserver::GetBraveShieldsRemote(
    content::RenderFrameHost* rfh) {
  if (!brave_shields_remotes_.contains(rfh)) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &brave_shields_remotes_[rfh]);
  }

  DCHECK(brave_shields_remotes_[rfh].is_bound());
  return brave_shields_remotes_[rfh];
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AllowCosmeticFilteringWebContentsObserver);

}  // namespace brave_shields
