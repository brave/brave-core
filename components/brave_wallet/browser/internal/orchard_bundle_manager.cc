// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"

namespace brave_wallet {

OrchardBundleManager::~OrchardBundleManager() = default;

// static
std::optional<size_t> OrchardBundleManager::random_seed_for_testing_ =
    std::nullopt;

// static
std::unique_ptr<OrchardBundleManager> OrchardBundleManager::Create(
    base::span<const uint8_t> tree_state,
    const std::vector<::brave_wallet::OrchardOutput>& orchard_outputs) {
  if (orchard_outputs.empty()) {
    return nullptr;
  }
  auto bundle = orchard::UnauthorizedOrchardBundle::Create(
      tree_state, orchard_outputs, random_seed_for_testing_);
  if (!bundle) {
    return nullptr;
  }
  return base::WrapUnique<OrchardBundleManager>(
      new OrchardBundleManager(std::move(bundle)));
}

OrchardBundleManager::OrchardBundleManager(
    std::unique_ptr<orchard::UnauthorizedOrchardBundle> unauthorized_bundle)
    : unauthorized_orchard_bundle_(std::move(unauthorized_bundle)) {}

OrchardBundleManager::OrchardBundleManager(
    std::unique_ptr<orchard::AuthorizedOrchardBundle> authorized_bundle)
    : authorized_orchard_bundle_(std::move(authorized_bundle)) {}

std::optional<std::array<uint8_t, kZCashDigestSize>>
OrchardBundleManager::GetOrchardDigest() {
  if (!unauthorized_orchard_bundle_) {
    return std::nullopt;
  }
  return unauthorized_orchard_bundle_->GetDigest();
}

// Apply tx signature digest to create zk-proof
std::unique_ptr<OrchardBundleManager> OrchardBundleManager::ApplySignature(
    std::array<uint8_t, kZCashDigestSize> sighash) {
  if (!unauthorized_orchard_bundle_) {
    return nullptr;
  }
  auto bundle = unauthorized_orchard_bundle_->Complete(sighash);
  if (!bundle) {
    return nullptr;
  }
  return base::WrapUnique<OrchardBundleManager>(
      new OrchardBundleManager(std::move(bundle)));
}

// Returns raw orchard bytes to use in transaction
std::optional<std::vector<uint8_t>> OrchardBundleManager::GetRawTxBytes() {
  if (!authorized_orchard_bundle_) {
    return std::nullopt;
  }
  return authorized_orchard_bundle_->GetOrchardRawTxPart();
}

}  // namespace brave_wallet
