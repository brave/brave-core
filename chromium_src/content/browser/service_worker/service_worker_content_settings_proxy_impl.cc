/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/browser/service_worker/service_worker_content_settings_proxy_impl.cc"

namespace content {

void ServiceWorkerContentSettingsProxyImpl::AllowFingerprinting(
    AllowFingerprintingCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // May be shutting down.
  if (!context_wrapper_->browser_context()) {
    std::move(callback).Run(false);
    return;
  }
  if (origin_.opaque()) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(
      GetContentClient()->browser()->AllowWorkerFingerprinting(
          origin_.GetURL(), context_wrapper_->browser_context()));
}

void ServiceWorkerContentSettingsProxyImpl::GetBraveFarblingLevel(
    GetBraveFarblingLevelCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // May be shutting down.
  if (!context_wrapper_->browser_context()) {
    std::move(callback).Run(1 /* OFF */);
    return;
  }
  if (origin_.opaque()) {
    std::move(callback).Run(1 /* OFF */);
    return;
  }
  std::move(callback).Run(
      GetContentClient()->browser()->WorkerGetBraveFarblingLevel(
          origin_.GetURL(), context_wrapper_->browser_context()));
}

}  // namespace content
