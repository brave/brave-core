/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/content_settings/brave_content_settings_manager_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace chrome {

BraveContentSettingsManagerImpl::~BraveContentSettingsManagerImpl() = default;

// static
void BraveContentSettingsManagerImpl::Create(
    content::RenderProcessHost* render_process_host,
    mojo::PendingReceiver<mojom::ContentSettingsManager> receiver) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(
          new BraveContentSettingsManagerImpl(render_process_host)),
      std::move(receiver));
}

BraveContentSettingsManagerImpl::BraveContentSettingsManagerImpl(
    content::RenderProcessHost* render_process_host)
    : ContentSettingsManagerImpl(render_process_host),
      host_content_settings_map_(HostContentSettingsMapFactory::GetForProfile(
          Profile::FromBrowserContext(
              render_process_host->GetBrowserContext()))) {}

BraveContentSettingsManagerImpl::BraveContentSettingsManagerImpl(
    const BraveContentSettingsManagerImpl& other)
    : ContentSettingsManagerImpl(other),
      host_content_settings_map_(other.host_content_settings_map_) {}

bool BraveContentSettingsManagerImpl::ShouldStoreState(
    int render_frame_id,
    const url::Origin& origin,
    const url::Origin& top_origin) {
  return g_brave_browser_process->tracking_protection_service()
      ->ShouldStoreState(host_content_settings_map_, render_process_id_,
                         render_frame_id, top_origin.GetURL(), origin.GetURL());
}

// mojom::ContentSettingsManager methods:
void BraveContentSettingsManagerImpl::Clone(
    mojo::PendingReceiver<mojom::ContentSettingsManager> receiver) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new BraveContentSettingsManagerImpl(*this)),
      std::move(receiver));
}

void BraveContentSettingsManagerImpl::AllowStorageAccess(
    int32_t render_frame_id,
    StorageType storage_type,
    const url::Origin& origin,
    const GURL& site_for_cookies,
    const url::Origin& top_frame_origin,
    base::OnceCallback<void(bool)> callback) {
  // LOCAL_STORAGE | SESSION_STORAGE == DOMStorage
  switch (storage_type) {
    case StorageType::DATABASE:
    case StorageType::LOCAL_STORAGE:
    case StorageType::SESSION_STORAGE:
    case StorageType::INDEXED_DB:
      if (!ShouldStoreState(render_frame_id, origin, top_frame_origin)) {
        std::move(callback).Run(false);
        return;
      }
      break;
    default:
      // FILE_SYSTEM, CACHE, WEB_LOCKS
      break;
  }

  ContentSettingsManagerImpl::AllowStorageAccess(
      render_frame_id, storage_type, origin, site_for_cookies, top_frame_origin,
      std::move(callback));
}

}  // namespace chrome
