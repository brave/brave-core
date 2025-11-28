// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_universal_web_contents_observers.h"

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"

void AttachBraveUniversalWebContentsObservers(
    content::WebContents* web_contents) {
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
      web_contents);
  if (base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage)) {
    ephemeral_storage::EphemeralStorageTabHelper::CreateForWebContents(
        web_contents);
  }
}
