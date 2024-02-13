/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/strings/strcat.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "storage/browser/blob/blob_url_store_impl.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace storage {
namespace {

// Checks if an origin is usable for partitioning. Origin is considered usable
// if it's opaque and has a valid precursor origin. Partitioning is done by
// appending an opaque nonce internally to all URLs, the valid precursor origin
// is used to check if an URL can be used to access a blob.
bool CanUseOriginForPartitioning(const url::Origin& origin) {
  return net::EphemeralStorageOriginUtils::CanUseNonceForEphemeralStorageKeying(
             origin) &&
         origin.GetTupleOrPrecursorTupleIfOpaque().IsValid() &&
         base::FeatureList::IsEnabled(
             net::features::kBravePartitionBlobStorage);
}

bool IsBlobUrlValidForPartitionedOrigin(const url::Origin& origin,
                                        const url::Origin& url_origin) {
  if (!CanUseOriginForPartitioning(origin)) {
    return false;
  }

  const url::Origin& non_opaque_origin =
      url::Origin::Create(origin.GetTupleOrPrecursorTupleIfOpaque().GetURL());
  return non_opaque_origin == url_origin;
}

}  // namespace
}  // namespace storage

#define BlobURLStoreImpl BlobURLStoreImpl_ChromiumImpl
#define BRAVE_BLOB_URL_STORE_IMPL_BLOB_URL_IS_VALID                         \
  if (!valid_origin) {                                                      \
    valid_origin =                                                          \
        IsBlobUrlValidForPartitionedOrigin(storage_key_origin, url_origin); \
  }

#include "src/storage/browser/blob/blob_url_store_impl.cc"

#undef BRAVE_BLOB_URL_STORE_IMPL_BLOB_URL_IS_VALID
#undef BlobURLStoreImpl

namespace storage {

BlobURLStoreImpl::BlobURLStoreImpl(
    const blink::StorageKey& storage_key,
    base::WeakPtr<BlobUrlRegistry> registry,
    BlobURLValidityCheckBehavior validity_check_behavior)
    : BlobURLStoreImpl_ChromiumImpl::BlobURLStoreImpl_ChromiumImpl(
          storage_key,
          std::move(registry),
          validity_check_behavior) {}

void BlobURLStoreImpl::Register(
    mojo::PendingRemote<blink::mojom::Blob> blob,
    const GURL& url,
    const base::UnguessableToken& unsafe_agent_cluster_id,
    const std::optional<net::SchemefulSite>& unsafe_top_level_site,
    RegisterCallback callback) {
  BlobURLStoreImpl_ChromiumImpl::Register(
      std::move(blob), GetPartitionedOrOriginalUrl(url),
      unsafe_agent_cluster_id, unsafe_top_level_site, std::move(callback));
}

void BlobURLStoreImpl::Revoke(const GURL& url) {
  BlobURLStoreImpl_ChromiumImpl::Revoke(GetPartitionedOrOriginalUrl(url));
}

void BlobURLStoreImpl::Resolve(const GURL& url, ResolveCallback callback) {
  BlobURLStoreImpl_ChromiumImpl::Resolve(GetPartitionedOrOriginalUrl(url),
                                         std::move(callback));
}

void BlobURLStoreImpl::ResolveAsURLLoaderFactory(
    const GURL& url,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
    ResolveAsURLLoaderFactoryCallback callback) {
  if (!registry_) {
    BlobURLLoaderFactory::Create(mojo::NullRemote(), url, std::move(receiver));
    std::move(callback).Run(std::nullopt, std::nullopt);
    return;
  }

  // Use modified URL only for accessing blob registry. Original URL is passed
  // as is into BlobURLLoaderFactory.
  const GURL& ephemeral_url = GetPartitionedOrOriginalUrl(url);
  BlobURLLoaderFactory::Create(registry_->GetBlobFromUrl(ephemeral_url), url,
                               std::move(receiver));
  std::move(callback).Run(registry_->GetUnsafeAgentClusterID(ephemeral_url),
                          registry_->GetUnsafeTopLevelSite(ephemeral_url));
}

void BlobURLStoreImpl::ResolveForNavigation(
    const GURL& url,
    mojo::PendingReceiver<blink::mojom::BlobURLToken> token,
    ResolveForNavigationCallback callback) {
  if (!registry_) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // Use modified URL only for accessing blob registry. Original URL is passed
  // as is into BlobURLTokenImpl.
  const GURL& ephemeral_url = GetPartitionedOrOriginalUrl(url);
  mojo::PendingRemote<blink::mojom::Blob> blob =
      registry_->GetBlobFromUrl(ephemeral_url);
  if (!blob) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  new BlobURLTokenImpl(registry_, url, std::move(blob), std::move(token));
  std::move(callback).Run(registry_->GetUnsafeAgentClusterID(ephemeral_url));
}

GURL BlobURLStoreImpl::GetPartitionedOrOriginalUrl(const GURL& url) const {
  const url::Origin& storage_key_origin = storage_key_.origin();
  if (!CanUseOriginForPartitioning(storage_key_origin)) {
    return url;
  }

  // Use origin nonce as a partition key and append it to the URL path.
  GURL clean_url = BlobUrlUtils::ClearUrlFragment(url);
  std::string partitioned_path = base::StrCat(
      {clean_url.path_piece(), "_",
       net::EphemeralStorageOriginUtils::GetNonceForEphemeralStorageKeying(
           storage_key_origin)
           .ToString()});
  GURL::Replacements replacements;
  replacements.SetPathStr(partitioned_path);
  return clean_url.ReplaceComponents(replacements);
}

}  // namespace storage
