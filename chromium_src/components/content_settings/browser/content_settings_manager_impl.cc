/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/browser/content_settings_manager_impl.h"

#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/page_load_metrics/browser/page_load_metrics_observer.h"

namespace content_settings {
namespace {
void OnStorageAccessed(int process_id,
                       int frame_id,
                       const GURL& origin_url,
                       const GURL& top_origin_url,
                       bool blocked_by_policy,
                       page_load_metrics::StorageType storage_type);

void OnDomStorageAccessed(int process_id,
                          int frame_id,
                          const GURL& origin_url,
                          const GURL& top_origin_url,
                          bool local,
                          bool blocked_by_policy);
}  // namespace

void ContentSettingsManagerImpl::AllowStorageAccessWithoutEphemeralStorage(
    int32_t render_frame_id,
    StorageType storage_type,
    const url::Origin& origin,
    const GURL& site_for_cookies,
    const url::Origin& top_frame_origin,
    base::OnceCallback<void(bool)> callback) {
  GURL url = origin.GetURL();
  bool allowed = cookie_settings_->IsCookieAccessAllowed(url, site_for_cookies,
                                                         top_frame_origin);
  if (delegate_->AllowStorageAccess(render_process_id_, render_frame_id,
                                    storage_type, url, allowed, &callback)) {
    DCHECK(!callback);
    return;
  }

  switch (storage_type) {
    case StorageType::DATABASE:
      PageSpecificContentSettings::WebDatabaseAccessed(
          render_process_id_, render_frame_id, url, !allowed);
      break;
    case StorageType::LOCAL_STORAGE:
      OnDomStorageAccessed(render_process_id_, render_frame_id, url,
                           top_frame_origin.GetURL(), true, !allowed);
      OnStorageAccessed(render_process_id_, render_frame_id, url,
                        top_frame_origin.GetURL(), !allowed,
                        page_load_metrics::StorageType::kLocalStorage);
      break;
    case StorageType::SESSION_STORAGE:
      OnDomStorageAccessed(render_process_id_, render_frame_id, url,
                           top_frame_origin.GetURL(), false, !allowed);
      OnStorageAccessed(render_process_id_, render_frame_id, url,
                        top_frame_origin.GetURL(), !allowed,
                        page_load_metrics::StorageType::kSessionStorage);

      break;
    case StorageType::FILE_SYSTEM:
      PageSpecificContentSettings::FileSystemAccessed(
          render_process_id_, render_frame_id, url, !allowed);
      OnStorageAccessed(render_process_id_, render_frame_id, url,
                        top_frame_origin.GetURL(), !allowed,
                        page_load_metrics::StorageType::kFileSystem);
      break;
    case StorageType::INDEXED_DB:
      PageSpecificContentSettings::IndexedDBAccessed(
          render_process_id_, render_frame_id, url, !allowed);
      OnStorageAccessed(render_process_id_, render_frame_id, url,
                        top_frame_origin.GetURL(), !allowed,
                        page_load_metrics::StorageType::kIndexedDb);
      break;
    case StorageType::CACHE:
      PageSpecificContentSettings::CacheStorageAccessed(
          render_process_id_, render_frame_id, url, !allowed);
      OnStorageAccessed(render_process_id_, render_frame_id, url,
                        top_frame_origin.GetURL(), !allowed,
                        page_load_metrics::StorageType::kCacheStorage);
      break;
    case StorageType::WEB_LOCKS:
      // State not persisted, no need to record anything.
      break;
  }

  std::move(callback).Run(allowed);
}
}  // namespace content_settings

#define IsCookieAccessAllowed IsCookieAccessOrEphemeralCookiesAccessAllowed

#include "../../../../../components/content_settings/browser/content_settings_manager_impl.cc"

#undef IsCookieAccessAllowed
