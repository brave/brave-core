/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/uphold/uphold_card.h"

#include <utility>

#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::uphold {

UpholdCard::UpholdCard(RewardsEngineImpl& engine)
    : engine_(engine), uphold_server_(engine) {}

UpholdCard::~UpholdCard() = default;

void UpholdCard::CreateBATCardIfNecessary(const std::string& access_token,
                                          CreateCardCallback callback) const {
  uphold_server_.get_cards().Request(
      access_token,
      base::BindOnce(&UpholdCard::OnGetBATCardId, base::Unretained(this),
                     std::move(callback), access_token));
}

void UpholdCard::OnGetBATCardId(CreateCardCallback callback,
                                const std::string& access_token,
                                mojom::Result result,
                                std::string&& id) const {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result == mojom::Result::OK && !id.empty()) {
    return std::move(callback).Run(mojom::Result::OK, std::move(id));
  }

  engine_->Log(FROM_HERE) << "Couldn't get BAT card ID";

  uphold_server_.post_cards().Request(
      access_token,
      base::BindOnce(&UpholdCard::OnCreateBATCard, base::Unretained(this),
                     std::move(callback), access_token));
}

void UpholdCard::OnCreateBATCard(CreateCardCallback callback,
                                 const std::string& access_token,
                                 mojom::Result result,
                                 std::string&& id) const {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Couldn't create BAT card";
    return std::move(callback).Run(result, "");
  }

  if (id.empty()) {
    engine_->LogError(FROM_HERE) << "BAT card ID is empty";
    return std::move(callback).Run(mojom::Result::FAILED, "");
  }

  uphold_server_.patch_card().Request(
      access_token, id,
      base::BindOnce(&UpholdCard::OnUpdateBATCardSettings,
                     base::Unretained(this), std::move(callback), id));
}

void UpholdCard::OnUpdateBATCardSettings(CreateCardCallback callback,
                                         std::string&& id,
                                         mojom::Result result) const {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Couldn't update BAT card settings";
    return std::move(callback).Run(result, "");
  }

  DCHECK(!id.empty());
  std::move(callback).Run(mojom::Result::OK, std::move(id));
}

}  // namespace brave_rewards::internal::uphold
