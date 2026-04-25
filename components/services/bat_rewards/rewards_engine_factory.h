/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_REWARDS_REWARDS_ENGINE_FACTORY_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_REWARDS_REWARDS_ENGINE_FACTORY_H_

#include "brave/components/services/bat_rewards/public/interfaces/rewards_engine_factory.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/self_owned_associated_receiver.h"

namespace brave_rewards::internal {

class RewardsEngineFactory : public mojom::RewardsEngineFactory {
 public:
  explicit RewardsEngineFactory(
      mojo::PendingReceiver<mojom::RewardsEngineFactory> receiver);

  ~RewardsEngineFactory() override;

  RewardsEngineFactory(const RewardsEngineFactory&) = delete;
  RewardsEngineFactory& operator=(const RewardsEngineFactory&) = delete;

  void CreateRewardsEngine(
      mojo::PendingAssociatedReceiver<mojom::RewardsEngine> engine_receiver,
      mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote,
      mojom::RewardsEngineOptionsPtr options,
      CreateRewardsEngineCallback callback) override;

 private:
  mojo::Receiver<mojom::RewardsEngineFactory> receiver_;
  mojo::SelfOwnedAssociatedReceiverRef<mojom::RewardsEngine> engine_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_REWARDS_REWARDS_ENGINE_FACTORY_H_
