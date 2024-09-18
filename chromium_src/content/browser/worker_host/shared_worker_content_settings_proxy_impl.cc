/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/browser/worker_host/shared_worker_content_settings_proxy_impl.cc"

namespace content {

void SharedWorkerContentSettingsProxyImpl::GetBraveShieldsSettings(
    GetBraveShieldsSettingsCallback callback) {
  if (!origin_.opaque()) {
    owner_->GetBraveShieldsSettings(origin_.GetURL(), std::move(callback));
  } else {
    std::move(callback).Run(brave_shields::mojom::ShieldsSettings::New());
  }
}

}  // namespace content
