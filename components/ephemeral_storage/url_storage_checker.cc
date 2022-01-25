/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/url_storage_checker.h"

#include <utility>

#include "components/services/storage/public/mojom/local_storage_control.mojom.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace ephemeral_storage {

UrlStorageChecker::UrlStorageChecker(
    content::StoragePartition* storage_partition,
    const GURL& url,
    Callback callback)
    : storage_partition_(storage_partition),
      url_(url),
      callback_(std::move(callback)) {
  DCHECK(storage_partition_);
  DCHECK(url_.is_valid());
  DCHECK(callback_);
}

UrlStorageChecker::~UrlStorageChecker() = default;

void UrlStorageChecker::StartCheck() {
  storage_partition_->GetCookieManagerForBrowserProcess()->GetCookieList(
      url_, net::CookieOptions::MakeAllInclusive(),
      net::CookiePartitionKeyCollection::ContainsAll(),
      base::BindOnce(&UrlStorageChecker::OnGetCookieList, this));
}

void UrlStorageChecker::OnGetCookieList(
    const std::vector<net::CookieWithAccessResult>& included_cookies,
    const std::vector<net::CookieWithAccessResult>& excluded_cookies) {
  if (!included_cookies.empty()) {
    std::move(callback_).Run(false);
    return;
  }

  storage_partition_->GetLocalStorageControl()->BindStorageArea(
      blink::StorageKey(url::Origin::Create(url_)),
      local_storage_area_.BindNewPipeAndPassReceiver());
  local_storage_area_->GetAll(
      {}, base::BindOnce(&UrlStorageChecker::OnGetLocalStorageData, this));
}

void UrlStorageChecker::OnGetLocalStorageData(
    std::vector<blink::mojom::KeyValuePtr> local_storage_data) {
  if (!local_storage_data.empty()) {
    std::move(callback_).Run(false);
    return;
  }

  std::move(callback_).Run(true);
}

}  // namespace ephemeral_storage
