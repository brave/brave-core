/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/check.h"
#include "base/synchronization/lock.h"
#include "base/template_util.h"
#include "base/thread_annotations.h"

#include "brave/browser/brave_rewards/vg_sync_service.h"

#include <tuple>
#include <utility>

namespace internal {

template <typename... Ts>
class WhenArgsReadyCallbackInfo {
 public:
  WhenArgsReadyCallbackInfo(base::OnceCallback<void(Ts...)> done)
      : done_(std::move(done)) {}

  template <typename T, std::size_t I>
  void Run(T t) LOCKS_EXCLUDED(mutex_) {
    base::ReleasableAutoLock lock(&mutex_);
    std::get<I>(args_) = std::move(t);

    if (std::apply([](auto&&... as) { return (... && as); }, args_)) {
      auto args = std::move(args_);
      lock.Release();
      std::apply(
          [done = std::move(done_)](auto... as) mutable {
            std::move(done).Run(*std::move(as)...);
          },
          std::move(args));
    }
  }

 private:
  base::Lock mutex_;
  std::tuple<absl::optional<Ts>...> args_ GUARDED_BY(mutex_);
  base::OnceCallback<void(Ts...)> done_;
};

template <typename... Ts, std::size_t... Is>
auto WhenArgsReadyCallback(base::OnceCallback<void(Ts...)> done,
                           std::index_sequence<Is...>) {
  auto info =
      std::make_shared<WhenArgsReadyCallbackInfo<Ts...>>(std::move(done));
  return std::tuple{
      base::BindOnce(&WhenArgsReadyCallbackInfo<Ts...>::template Run<Ts, Is>,
                     Is == sizeof...(Ts) - 1 ? std::move(info) : info)...};
}

}  // namespace internal

template <typename... Ts,
          template <typename>
          typename CallbackType,
          typename = base::EnableIfIsBaseCallback<CallbackType>>
std::tuple<base::OnceCallback<void(Ts)>...> WhenArgsReadyCallback(
    CallbackType<void(Ts...)> done) {
  return internal::WhenArgsReadyCallback(std::move(done),
                                         std::index_sequence_for<Ts...>());
}

namespace brave_rewards {
VgSyncService::VgSyncService(
    std::unique_ptr<VgBodySyncBridge> vg_body_sync_bridge,
    std::unique_ptr<VgSpendStatusSyncBridge> vg_spend_status_sync_bridge)
    : vg_body_sync_bridge_(
          (DCHECK(vg_body_sync_bridge), std::move(vg_body_sync_bridge))),
      vg_spend_status_sync_bridge_((DCHECK(vg_spend_status_sync_bridge),
                                    std::move(vg_spend_status_sync_bridge))) {}

VgSyncService::~VgSyncService() = default;

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgSyncService::GetControllerDelegateForVgBodies() {
  DCHECK(vg_body_sync_bridge_);
  return vg_body_sync_bridge_ ? vg_body_sync_bridge_->GetControllerDelegate()
                              : nullptr;
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgSyncService::GetControllerDelegateForVgSpendStatuses() {
  DCHECK(vg_spend_status_sync_bridge_);
  return vg_spend_status_sync_bridge_
             ? vg_spend_status_sync_bridge_->GetControllerDelegate()
             : nullptr;
}

void VgSyncService::Shutdown() {}

void VgSyncService::BackUpVgBodies(
    std::vector<sync_pb::VgBodySpecifics> vg_bodies) {
  vg_body_sync_bridge_->BackUpVgBodies(std::move(vg_bodies));
}

void VgSyncService::BackUpVgSpendStatuses(
    std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses) {
  vg_spend_status_sync_bridge_->BackUpVgSpendStatuses(
      std::move(vg_spend_statuses));
}

void VgSyncService::SetCallback(
    base::OnceCallback<void(std::vector<sync_pb::VgBodySpecifics>,
                            std::vector<sync_pb::VgSpendStatusSpecifics>)>
        restore_vgs) {
  auto [vg_bodies_cb, vg_spend_statuses_cb] =
      WhenArgsReadyCallback(std::move(restore_vgs));
  vg_body_sync_bridge_->SetCallback(std::move(vg_bodies_cb));
  vg_spend_status_sync_bridge_->SetCallback(std::move(vg_spend_statuses_cb));
}
}  // namespace brave_rewards
