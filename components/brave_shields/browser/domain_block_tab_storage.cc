/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/domain_block_tab_storage.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/web_contents.h"

namespace brave_shields {

// Arbitrary but unique key required for SupportsUserData.
// Upstream does this too.
const void* const kDomainBlockTabStorageKey = &kDomainBlockTabStorageKey;

DomainBlockTabStorage::DomainBlockTabStorage(content::WebContents* contents)
    : content::WebContentsUserData<DomainBlockTabStorage>(*contents) {}

DomainBlockTabStorage::~DomainBlockTabStorage() = default;

// static
DomainBlockTabStorage* DomainBlockTabStorage::GetOrCreate(
    content::WebContents* web_contents) {
  DomainBlockTabStorage* storage = FromWebContents(web_contents);
  if (!storage) {
    CreateForWebContents(web_contents);
    storage = FromWebContents(web_contents);
  }
  return storage;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(DomainBlockTabStorage);

}  // namespace brave_shields
