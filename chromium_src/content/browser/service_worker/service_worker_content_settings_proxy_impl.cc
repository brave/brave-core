/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/content_settings.mojom.h"
#include "content/browser/storage_partition_impl.h"

#include <content/browser/service_worker/service_worker_content_settings_proxy_impl.cc>

namespace content {

void ServiceWorkerContentSettingsProxyImpl::GetBraveShieldsSettings(
    GetBraveShieldsSettingsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // May be shutting down.
  if (!context_wrapper_->browser_context()) {
    std::move(callback).Run(brave_shields::mojom::ShieldsSettings::New());
    return;
  }
  // Shields should also work in opaque origins.
  const GURL url = origin_.GetTupleOrPrecursorTupleIfOpaque().GetURL();
  const StoragePartitionConfig* storage_partition_config = nullptr;
  if (StoragePartitionImpl* storage_partition =
          context_wrapper_->storage_partition()) {
    storage_partition_config = &storage_partition->GetConfig();
  }
  std::move(callback).Run(
      GetContentClient()->browser()->WorkerGetBraveShieldSettings(
          url, context_wrapper_->browser_context(), storage_partition_config));
}

void ServiceWorkerContentSettingsProxyImpl::GetBraveFarblingLevel(
    int32_t webcompat_settings_type,
    GetBraveFarblingLevelCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!context_wrapper_->browser_context()) {
    std::move(callback).Run(
        static_cast<int32_t>(content_settings::mojom::FarblingLevel::OFF));
    return;
  }
  const GURL url = origin_.GetTupleOrPrecursorTupleIfOpaque().GetURL();
  std::move(callback).Run(
      GetContentClient()->browser()->WorkerGetBraveFarblingLevel(
          url, context_wrapper_->browser_context(), webcompat_settings_type));
}

}  // namespace content
