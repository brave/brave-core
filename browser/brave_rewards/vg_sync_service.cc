/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_sync_service.h"
#include "base/barrier_callback.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/check.h"
#include "base/synchronization/lock.h"
#include "base/template_util.h"
#include "base/thread_annotations.h"

#include "third_party/abseil-cpp/absl/types/variant.h"

#include <tuple>
#include <utility>

template <typename... Ts,
          template <typename>
          typename CallbackType,
          typename = base::EnableIfIsBaseCallback<CallbackType>>
auto WhenArgsReadyCallback(CallbackType<void(Ts...)> done) {
  auto arg_cb = base::BarrierCallback<absl::variant<Ts...>>(
      sizeof...(Ts),
      base::BindOnce(
          [](base::OnceCallback<void(Ts...)> done,
             std::vector<absl::variant<Ts...>> v) {
            std::tuple<Ts...> args;

            for (auto& arg : v) {
              absl::visit(
                  [&](auto a) { std::get<decltype(a)>(args) = std::move(a); },
                  std::move(arg));
            }

            std::apply(
                [done = std::move(done)](auto... as) mutable {
                  std::move(done).Run(std::move(as)...);
                },
                std::move(args));
          },
          std::move(done)));

  return std::tuple{
      base::BindOnce([](base::OnceCallback<void(absl::variant<Ts...>)> arg_cb,
                        Ts ts) { std::move(arg_cb).Run(std::move(ts)); },
                     arg_cb)...};
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
