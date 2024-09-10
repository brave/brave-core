/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/browser/service_worker/service_worker_content_settings_proxy_impl.cc"

namespace content {

void ServiceWorkerContentSettingsProxyImpl::GetBraveShieldsSettings(
    GetBraveShieldsSettingsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // May be shutting down.
  if (!context_wrapper_->browser_context()) {
    std::move(callback).Run(brave_shields::mojom::ShieldsSettings::New());
    return;
  }
  if (origin_.opaque()) {
    std::move(callback).Run(brave_shields::mojom::ShieldsSettings::New());
    return;
  }
  std::move(callback).Run(
      GetContentClient()->browser()->WorkerGetBraveShieldSettings(
          origin_.GetURL(), context_wrapper_->browser_context()));
}

}  // namespace content
