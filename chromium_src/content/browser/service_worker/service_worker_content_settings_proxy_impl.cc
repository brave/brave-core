/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../content/browser/service_worker/service_worker_content_settings_proxy_impl.cc"

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
  // |render_frames| is used to show UI for the frames affected by the
  // content setting. However, service worker is not necessarily associated
  // with frames or making the request on behalf of frames,
  // so just pass an empty |render_frames|.
  std::vector<GlobalRenderFrameHostId> render_frames;
  std::move(callback).Run(
      GetContentClient()->browser()->AllowWorkerFingerprinting(
          origin_.GetURL(), context_wrapper_->browser_context(),
          render_frames));
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
  // |render_frames| is used to show UI for the frames affected by the
  // content setting. However, service worker is not necessarily associated
  // with frames or making the request on behalf of frames,
  // so just pass an empty |render_frames|.
  std::vector<GlobalRenderFrameHostId> render_frames;
  std::move(callback).Run(
      GetContentClient()->browser()->WorkerGetBraveFarblingLevel(
          origin_.GetURL(), context_wrapper_->browser_context(),
          render_frames));
}

}  // namespace content
