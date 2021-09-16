/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/services/storage/dom_storage/local_storage_impl.h"

#define LocalStorageImpl LocalStorageImpl_ChromiumImpl

#include "../../../../../../components/services/storage/dom_storage/local_storage_impl.cc"

#undef LocalStorageImpl

namespace storage {

LocalStorageImpl::LocalStorageImpl(
    const base::FilePath& storage_root,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    mojo::PendingReceiver<mojom::LocalStorageControl> receiver)
    : local_storage_(std::make_unique<LocalStorageImpl_ChromiumImpl>(
          storage_root,
          task_runner,
          mojo::PendingReceiver<mojom::LocalStorageControl>())),
      in_memory_local_storage_(std::make_unique<LocalStorageImpl_ChromiumImpl>(
          base::FilePath(),
          task_runner,
          mojo::PendingReceiver<mojom::LocalStorageControl>())) {
  if (receiver)
    control_receiver_.Bind(std::move(receiver));
}

LocalStorageImpl::~LocalStorageImpl() = default;

void LocalStorageImpl::ShutDown(base::OnceClosure callback) {
  local_storage_->ShutDown(
      base::BindOnce(&LocalStorageImpl::ShutDownInMemoryStorage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void LocalStorageImpl::BindStorageArea(
    const blink::StorageKey& storage_key,
    mojo::PendingReceiver<blink::mojom::StorageArea> receiver) {
  if (storage_key.origin().opaque()) {
    in_memory_local_storage_->BindStorageArea(
        *GetStorageKeyWithNonOpaqueOrigin(storage_key, true),
        std::move(receiver));
  } else {
    local_storage_->BindStorageArea(storage_key, std::move(receiver));
  }
}

void LocalStorageImpl::GetUsage(GetUsageCallback callback) {
  local_storage_->GetUsage(std::move(callback));
}

void LocalStorageImpl::DeleteStorage(const blink::StorageKey& storage_key,
                                     DeleteStorageCallback callback) {
  const url::Origin& storage_key_origin = storage_key.origin();
  if (storage_key_origin.opaque()) {
    if (const auto* non_opaque_origins_storage_key =
            GetStorageKeyWithNonOpaqueOrigin(storage_key, false)) {
      in_memory_local_storage_->DeleteStorage(*non_opaque_origins_storage_key,
                                              std::move(callback));
      storage_keys_with_non_opaque_origin_.erase(storage_key_origin);
    } else {
      std::move(callback).Run();
    }
  } else {
    local_storage_->DeleteStorage(storage_key, std::move(callback));
  }
}

void LocalStorageImpl::CleanUpStorage(CleanUpStorageCallback callback) {
  local_storage_->CleanUpStorage(std::move(callback));
}

void LocalStorageImpl::Flush(FlushCallback callback) {
  local_storage_->Flush(std::move(callback));
}

void LocalStorageImpl::PurgeMemory() {
  local_storage_->PurgeMemory();
}

void LocalStorageImpl::ApplyPolicyUpdates(
    std::vector<mojom::StoragePolicyUpdatePtr> policy_updates) {
  local_storage_->ApplyPolicyUpdates(std::move(policy_updates));
}

void LocalStorageImpl::ForceKeepSessionState() {
  local_storage_->ForceKeepSessionState();
}

const blink::StorageKey* LocalStorageImpl::GetStorageKeyWithNonOpaqueOrigin(
    const blink::StorageKey& storage_key,
    bool create) {
  const url::Origin& storage_key_origin = storage_key.origin();
  DCHECK(storage_key_origin.opaque());
  auto storage_keys_it =
      storage_keys_with_non_opaque_origin_.find(storage_key_origin);
  if (storage_keys_it != storage_keys_with_non_opaque_origin_.end()) {
    return &storage_keys_it->second;
  }
  if (!create) {
    return nullptr;
  }

  const auto& origin_scheme_host_port =
      storage_key_origin.GetTupleOrPrecursorTupleIfOpaque();
  url::Origin non_opaque_origin = url::Origin::CreateFromNormalizedTuple(
      origin_scheme_host_port.scheme(),
      base::ToLowerASCII(base::UnguessableToken::Create().ToString()),
      origin_scheme_host_port.port());
  auto emplaced_pair = storage_keys_with_non_opaque_origin_.emplace(
      storage_key_origin, blink::StorageKey(non_opaque_origin));
  return &emplaced_pair.first->second;
}

void LocalStorageImpl::ShutDownInMemoryStorage(base::OnceClosure callback) {
  in_memory_local_storage_->ShutDown(std::move(callback));
}

}  // namespace storage
