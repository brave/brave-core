/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/image_fetcher/image_fetcher_service_factory.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "chrome/browser/transition_manager/full_browser_transition_manager.h"
#include "components/image_fetcher/core/cache/image_data_store.h"
#include "components/image_fetcher/core/cache/image_store_types.h"
#include "components/image_fetcher/core/image_fetcher_types.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/network/public/cpp/cross_thread_pending_shared_url_loader_factory.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace {

// Forwards requests to an off-the-record profile's own URLLoaderFactory. The
// service is built from a SimpleFactoryKey, possibly before the Profile
// exists, so the profile is looked up when its first request starts. Requests
// started before the profile exists, or after it is destroyed, are dropped,
// which their loaders report as a connection error.
class OffTheRecordProfileURLLoaderFactory
    : public network::SharedURLLoaderFactory {
 public:
  explicit OffTheRecordProfileURLLoaderFactory(SimpleFactoryKey* key) {
    FullBrowserTransitionManager::Get()->RegisterCallbackOnProfileCreation(
        key,
        base::BindOnce(&OffTheRecordProfileURLLoaderFactory::OnProfileCreated,
                       weak_factory_.GetWeakPtr()));
  }

  OffTheRecordProfileURLLoaderFactory(
      const OffTheRecordProfileURLLoaderFactory&) = delete;
  OffTheRecordProfileURLLoaderFactory& operator=(
      const OffTheRecordProfileURLLoaderFactory&) = delete;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> receiver,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& url_request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (auto* factory = GetProfileFactory()) {
      factory->CreateLoaderAndStart(std::move(receiver), request_id, options,
                                    url_request, std::move(client),
                                    traffic_annotation);
    }
  }

  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (auto* factory = GetProfileFactory()) {
      factory->Clone(std::move(receiver));
    }
  }

  // network::SharedURLLoaderFactory:
  std::unique_ptr<network::PendingSharedURLLoaderFactory> Clone() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return std::make_unique<network::CrossThreadPendingSharedURLLoaderFactory>(
        this);
  }

 private:
  friend class base::RefCounted<OffTheRecordProfileURLLoaderFactory>;
  ~OffTheRecordProfileURLLoaderFactory() override = default;

  void OnProfileCreated(Profile* profile) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    profile_ = profile->GetWeakPtr();
  }

  network::SharedURLLoaderFactory* GetProfileFactory() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!profile_) {
      return nullptr;
    }
    return profile_->GetURLLoaderFactory().get();
  }

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtr<Profile> profile_;
  base::WeakPtrFactory<OffTheRecordProfileURLLoaderFactory> weak_factory_{this};
};

// An image data store that holds nothing. An off-the-record key keeps the
// regular profile's path, so its disk store would read the regular profile's
// cached images.
class EmptyImageDataStore : public image_fetcher::ImageDataStore {
 public:
  EmptyImageDataStore() = default;
  EmptyImageDataStore(const EmptyImageDataStore&) = delete;
  EmptyImageDataStore& operator=(const EmptyImageDataStore&) = delete;
  ~EmptyImageDataStore() override = default;

  // image_fetcher::ImageDataStore:
  void Initialize(base::OnceClosure callback) override {
    initialized_ = true;
    std::move(callback).Run();
  }
  bool IsInitialized() override { return initialized_; }
  void SaveImage(const std::string& key,
                 image_fetcher::ImageData image_data,
                 bool needs_transcoding) override {}
  void LoadImage(const std::string& key,
                 bool needs_transcoding,
                 image_fetcher::ImageDataCallback callback) override {
    std::move(callback).Run(/*needs_transcoding=*/false, std::string());
  }
  void DeleteImage(const std::string& key) override {}
  void GetAllKeys(image_fetcher::StoreKeysCallback callback) override {
    std::move(callback).Run(std::vector<std::string>());
  }

 private:
  bool initialized_ = false;
};

// Off-the-record keys get an image data store that holds nothing, and regular
// keys keep `disk_store`.
std::unique_ptr<image_fetcher::ImageDataStore> MaybeReplaceDataStore(
    SimpleFactoryKey* key,
    std::unique_ptr<image_fetcher::ImageDataStore> disk_store) {
  if (key->IsOffTheRecord()) {
    return std::make_unique<EmptyImageDataStore>();
  }
  return disk_store;
}

// An off-the-record profile's fetches go through its own network context, so
// they stay in its network partition and, in a Tor window, go through Tor.
scoped_refptr<network::SharedURLLoaderFactory> MaybeGetOffTheRecordFactory(
    SimpleFactoryKey* key) {
  if (!key->IsOffTheRecord()) {
    return nullptr;
  }
  return base::MakeRefCounted<OffTheRecordProfileURLLoaderFactory>(key);
}

}  // namespace

#include <chrome/browser/image_fetcher/image_fetcher_service_factory.cc>
