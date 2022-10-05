/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_tab_storage.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "content/public/browser/web_contents.h"

namespace request_otr {

// Arbitrary but unique key required for SupportsUserData.
// Upstream does this too.
const void* const kRequestOTRTabStorageKey = &kRequestOTRTabStorageKey;

RequestOTRTabStorage::RequestOTRTabStorage(content::WebContents* contents)
    : content::WebContentsUserData<RequestOTRTabStorage>(*contents) {}

RequestOTRTabStorage::~RequestOTRTabStorage() = default;

// static
RequestOTRTabStorage* RequestOTRTabStorage::GetOrCreate(
    content::WebContents* web_contents) {
  RequestOTRTabStorage* storage = FromWebContents(web_contents);
  if (!storage) {
    CreateForWebContents(web_contents);
    storage = FromWebContents(web_contents);
  }
  return storage;
}

void RequestOTRTabStorage::Enable1PESForUrlIfPossible(
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    const GURL& url,
    base::OnceCallback<void()> on_ready) {
  if (url.HostIsIPAddress()) {
    std::move(on_ready).Run();
    return;
  }

  DCHECK(ephemeral_storage_service);
  blocked_domain_1pes_lifetime_ =
      BlockedDomain1PESLifetime::GetOrCreate(ephemeral_storage_service, url);
  blocked_domain_1pes_lifetime_->AddOnReadyCallback(std::move(on_ready));
  LOG(ERROR) << "RequestOTRTabStorage::Enable1PESForUrlIfPossible successful!";
}

void RequestOTRTabStorage::DropBlockedDomain1PESLifetime() {
  blocked_domain_1pes_lifetime_.reset();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(RequestOTRTabStorage);

}  // namespace request_otr
