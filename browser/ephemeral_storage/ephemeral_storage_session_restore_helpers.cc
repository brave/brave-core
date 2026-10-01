/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

// Declared in chromium_src/chrome/browser/sessions/session_restore.cc.
bool BraveIsScheduledForCleanup(const GURL& url, Profile* profile) {
  auto* service = EphemeralStorageServiceFactory::GetForContext(profile);
  if (!service) {
    return false;
  }
  return service->IsScheduledForCleanup(net::URLToEphemeralStorageDomain(url));
}
