/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_

namespace storage {
class BlobURLStoreImpl;
using BlobURLStoreImpl_BraveImpl = BlobURLStoreImpl;
}  // namespace storage

#define BlobURLStoreImpl BlobURLStoreImpl_ChromiumImpl
#define BlobUrlIsValid               \
  NotUsed() const;                   \
  friend BlobURLStoreImpl_BraveImpl; \
  bool BlobUrlIsValid

#include <optional>

#include "src/storage/browser/blob/blob_url_store_impl.h"  // IWYU pragma: export

#undef BlobUrlIsValid
#undef BlobURLStoreImpl

namespace storage {

class COMPONENT_EXPORT(STORAGE_BROWSER) BlobURLStoreImpl
    : public BlobURLStoreImpl_ChromiumImpl {
 public:
  using BlobURLStoreImpl_ChromiumImpl::BlobURLStoreImpl_ChromiumImpl;

  void ResolveAsURLLoaderFactory(
      const GURL& url,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      ResolveAsURLLoaderFactoryCallback callback) override;
  void ResolveForNavigation(
      const GURL& url,
      mojo::PendingReceiver<blink::mojom::BlobURLToken> token,
      ResolveForNavigationCallback callback) override;

 private:
  bool IsBlobResolvable(const GURL& url) const;
};

}  // namespace storage

#endif  // BRAVE_CHROMIUM_SRC_STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_
