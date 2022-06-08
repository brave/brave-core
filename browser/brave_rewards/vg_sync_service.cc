/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_sync_service.h"

#include <utility>

#include "base/barrier_callback.h"

namespace {
using pair = std::pair<std::vector<sync_pb::VgBodySpecifics>,
                       std::vector<sync_pb::VgSpendStatusSpecifics>>;

using variant = absl::variant<pair::first_type, pair::second_type>;

auto WhenVgsReady(base::OnceCallback<void(pair::first_type, pair::second_type)>
                      when_vgs_ready) {
  auto arg_cb = base::BarrierCallback<variant>(
      2, base::BindOnce(
             [](base::OnceCallback<void(pair::first_type, pair::second_type)>
                    when_vgs_ready,
                std::vector<variant> args) {
               pair vg_bodies_vg_spend_statuses;

               for (auto& arg : args) {
                 absl::visit(
                     [&](auto arg) {
                       std::get<decltype(arg)>(vg_bodies_vg_spend_statuses) =
                           std::move(arg);
                     },
                     std::move(arg));
               }

               std::move(when_vgs_ready)
                   .Run(std::move(vg_bodies_vg_spend_statuses.first),
                        std::move(vg_bodies_vg_spend_statuses.second));
             },
             std::move(when_vgs_ready)));

  return std::pair{
      base::BindOnce(
          [](base::OnceCallback<void(variant)> when_vg_bodies_ready,
             pair::first_type vg_bodies) {
            std::move(when_vg_bodies_ready).Run(std::move(vg_bodies));
          },
          arg_cb),
      base::BindOnce(
          [](base::OnceCallback<void(variant)> when_vg_spend_statuses_ready,
             pair::second_type vg_spend_statuses) {
            std::move(when_vg_spend_statuses_ready)
                .Run(std::move(vg_spend_statuses));
          },
          arg_cb)};
}
}  // namespace

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

void VgSyncService::BackUpVgBodies(
    std::vector<sync_pb::VgBodySpecifics> vg_bodies) {
  vg_body_sync_bridge_->BackUpVgBodies(std::move(vg_bodies));
}

void VgSyncService::BackUpVgSpendStatuses(
    std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses) {
  vg_spend_status_sync_bridge_->BackUpVgSpendStatuses(
      std::move(vg_spend_statuses));
}

void VgSyncService::WhenVgsReady(
    base::RepeatingCallback<void(std::vector<sync_pb::VgBodySpecifics>,
                                 std::vector<sync_pb::VgSpendStatusSpecifics>)>
        when_vgs_ready) {
  auto [when_vg_bodies_ready, when_vg_spend_statuses_ready] =
      ::WhenVgsReady(when_vgs_ready.Then(
          base::BindRepeating(&VgSyncService::WhenVgsReady,
                              weak_ptr_factory_.GetWeakPtr(), when_vgs_ready)));

  vg_body_sync_bridge_->WhenVgBodiesReady(std::move(when_vg_bodies_ready));
  vg_spend_status_sync_bridge_->WhenVgSpendStatusesReady(
      std::move(when_vg_spend_statuses_ready));
}
}  // namespace brave_rewards
