/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SERVICES_STORAGE_DOM_STORAGE_LOCAL_STORAGE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SERVICES_STORAGE_DOM_STORAGE_LOCAL_STORAGE_IMPL_H_

#define LocalStorageImpl LocalStorageImpl_ChromiumImpl

#include "../../../../../../components/services/storage/dom_storage/local_storage_impl.h"

#undef LocalStorageImpl

namespace storage {

class LocalStorageImpl : public mojom::LocalStorageControl {
 public:
  LocalStorageImpl(const base::FilePath& storage_root,
                   scoped_refptr<base::SequencedTaskRunner> task_runner,
                   mojo::PendingReceiver<mojom::LocalStorageControl> receiver);
  ~LocalStorageImpl() override;

  void ShutDown(base::OnceClosure callback);

  // mojom::LocalStorageControl implementation:
  void BindStorageArea(
      const blink::StorageKey& storage_key,
      mojo::PendingReceiver<blink::mojom::StorageArea> receiver) override;
  void GetUsage(GetUsageCallback callback) override;
  void DeleteStorage(const blink::StorageKey& storage_key,
                     DeleteStorageCallback callback) override;
  void CleanUpStorage(CleanUpStorageCallback callback) override;
  void Flush(FlushCallback callback) override;
  void PurgeMemory() override;
  void ApplyPolicyUpdates(
      std::vector<mojom::StoragePolicyUpdatePtr> policy_updates) override;
  void ForceKeepSessionState() override;

 private:
  const blink::StorageKey* GetStorageKeyWithNonOpaqueOrigin(
      const blink::StorageKey& storage_key,
      bool create);

  void ShutDownInMemoryStorage(base::OnceClosure callback);

  std::unique_ptr<LocalStorageImpl_ChromiumImpl> local_storage_;
  std::unique_ptr<LocalStorageImpl_ChromiumImpl> in_memory_local_storage_;
  mojo::Receiver<mojom::LocalStorageControl> control_receiver_{this};
  // LocalStorageImpl works only with non-opaque origins, that's why we use a
  // map of opaque origins to StorageKeys with non-opaque origins.
  std::map<url::Origin, blink::StorageKey> storage_keys_with_non_opaque_origin_;

  base::WeakPtrFactory<LocalStorageImpl> weak_ptr_factory_{this};
};

}  // namespace storage

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SERVICES_STORAGE_DOM_STORAGE_LOCAL_STORAGE_IMPL_H_
