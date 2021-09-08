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
    const url::Origin& origin,
    mojo::PendingReceiver<blink::mojom::StorageArea> receiver) {
  if (origin.opaque()) {
    in_memory_local_storage_->BindStorageArea(*GetNonOpaqueOrigin(origin, true),
                                              std::move(receiver));
  } else {
    local_storage_->BindStorageArea(origin, std::move(receiver));
  }
}

void LocalStorageImpl::GetUsage(GetUsageCallback callback) {
  local_storage_->GetUsage(std::move(callback));
}

void LocalStorageImpl::DeleteStorage(const url::Origin& origin,
                                     DeleteStorageCallback callback) {
  if (origin.opaque()) {
    if (const auto* non_opaque_origin = GetNonOpaqueOrigin(origin, false)) {
      in_memory_local_storage_->DeleteStorage(*non_opaque_origin,
                                              std::move(callback));
      non_opaque_origins_.erase(origin);
    } else {
      std::move(callback).Run();
    }
  } else {
    local_storage_->DeleteStorage(origin, std::move(callback));
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

const url::Origin* LocalStorageImpl::GetNonOpaqueOrigin(
    const url::Origin& origin,
    bool create) {
  DCHECK(origin.opaque());
  auto non_opaque_origin_it = non_opaque_origins_.find(origin);
  if (non_opaque_origin_it != non_opaque_origins_.end()) {
    return &non_opaque_origin_it->second;
  }
  if (!create) {
    return nullptr;
  }

  const auto& origin_scheme_host_port =
      origin.GetTupleOrPrecursorTupleIfOpaque();
  auto emplaced_pair = non_opaque_origins_.emplace(
      origin,
      url::Origin::CreateFromNormalizedTuple(
          origin_scheme_host_port.scheme(),
          base::ToLowerASCII(base::UnguessableToken::Create().ToString()),
          origin_scheme_host_port.port()));
  return &emplaced_pair.first->second;
}

void LocalStorageImpl::ShutDownInMemoryStorage(base::OnceClosure callback) {
  in_memory_local_storage_->ShutDown(std::move(callback));
}

}  // namespace storage
