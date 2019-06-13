/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/plugins/brave_plugin_service_filter.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/plugin_service.h"

// static
BravePluginServiceFilter* BravePluginServiceFilter::GetInstance() {
  return base::Singleton<BravePluginServiceFilter>::get();
}

BravePluginServiceFilter::BravePluginServiceFilter()
  : ChromePluginServiceFilter() {
}

BravePluginServiceFilter::~BravePluginServiceFilter() {
}

// Plugin list caches are purged for a normal profile and its associated
// OTR profile when NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED notification
// is received in chromium, purge the cache for associated Tor profile here.
void BravePluginServiceFilter::Observe(int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  ChromePluginServiceFilter::Observe(type, source, details);
  if (type != chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED)
    return;

  Profile* profile = content::Source<Profile>(source).ptr();
  if (profile && profile->HasTorProfile()) {
    content::PluginService::GetInstance()->PurgePluginListCache(
        profile->GetTorProfile(), false);
  }
}
