/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "storage/browser/blob/blob_url_store_impl.h"

#include "storage/browser/blob/blob_url_utils.h"

#define BlobURLStoreImpl BlobURLStoreImpl_ChromiumImpl

#include "src/storage/browser/blob/blob_url_store_impl.cc"

#undef BlobURLStoreImpl

namespace storage {

void BlobURLStoreImpl::ResolveAsURLLoaderFactory(
    const GURL& url,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    ResolveAsURLLoaderFactoryCallback callback) {
  if (!IsBlobResolvable(url)) {
    std::move(callback).Run(std::nullopt, std::nullopt);
    return;
  }

  BlobURLStoreImpl_ChromiumImpl::ResolveAsURLLoaderFactory(
      url, std::move(receiver), std::move(callback));
}

void BlobURLStoreImpl::ResolveForNavigation(
    const GURL& url,
    mojo::PendingReceiver<blink::mojom::BlobURLToken> token,
    ResolveForNavigationCallback callback) {
  if (!IsBlobResolvable(url)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  BlobURLStoreImpl_ChromiumImpl::ResolveForNavigation(url, std::move(token),
                                                      std::move(callback));
}

bool BlobURLStoreImpl::IsBlobResolvable(const GURL& url) const {
  // Check if the URL is mapped to a BlobURLStore with the current
  // `storage_key_` or if it's an extension-generated blob.
  const GURL& clean_url = BlobUrlUtils::UrlHasFragment(url)
                              ? BlobUrlUtils::ClearUrlFragment(url)
                              : url;
  constexpr std::string_view kChromeExtensionScheme = "chrome-extension";
  return (registry_ && registry_->IsUrlMapped(clean_url, storage_key_)) ||
         (url.SchemeIsBlob() &&
          (url::Origin::Create(url).scheme() == kChromeExtensionScheme ||
           storage_key_.origin().scheme() == kChromeExtensionScheme));
}

}  // namespace storage
