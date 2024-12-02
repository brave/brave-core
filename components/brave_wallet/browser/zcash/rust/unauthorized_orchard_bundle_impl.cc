/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/unauthorized_orchard_bundle_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/zcash/rust/authorized_orchard_bundle_impl.h"

namespace brave_wallet::orchard {

UnauthorizedOrchardBundleImpl::UnauthorizedOrchardBundleImpl(
    ::rust::Box<CxxOrchardUnauthorizedBundle> orchard_unauthorized_bundle)
    : orchard_unauthorized_bundle_(std::move(orchard_unauthorized_bundle)) {}

UnauthorizedOrchardBundleImpl::~UnauthorizedOrchardBundleImpl() = default;

// static
std::unique_ptr<UnauthorizedOrchardBundle> UnauthorizedOrchardBundle::Create(
    base::span<const uint8_t> tree_state,
    const std::vector<::brave_wallet::OrchardOutput>& orchard_outputs,
    std::optional<size_t> random_seed_for_testing) {
  ::rust::Vec<orchard::CxxOrchardOutput> outputs;
  for (const auto& output : orchard_outputs) {
    outputs.push_back(orchard::CxxOrchardOutput{
        output.value, output.addr, output.memo ? *output.memo : OrchardMemo(),
        output.memo.has_value()});
  }
  if (random_seed_for_testing) {
    CHECK_IS_TEST();
    auto bundle_result = create_testing_orchard_bundle(
        ::rust::Slice<const uint8_t>{tree_state.data(), tree_state.size()},
        ::rust::Vec<::brave_wallet::orchard::CxxOrchardSpend>(),
        std::move(outputs), random_seed_for_testing.value());
    if (!bundle_result->is_ok()) {
      return nullptr;
    }
    return base::WrapUnique<UnauthorizedOrchardBundle>(
        new UnauthorizedOrchardBundleImpl(bundle_result->unwrap()));
  } else {
    auto bundle_result = create_orchard_bundle(
        ::rust::Slice<const uint8_t>{tree_state.data(), tree_state.size()},
        ::rust::Vec<::brave_wallet::orchard::CxxOrchardSpend>(),
        std::move(outputs));
    if (!bundle_result->is_ok()) {
      return nullptr;
    }
    return base::WrapUnique<UnauthorizedOrchardBundle>(
        new UnauthorizedOrchardBundleImpl(bundle_result->unwrap()));
  }
}

std::array<uint8_t, kZCashDigestSize>
UnauthorizedOrchardBundleImpl::GetDigest() {
  return orchard_unauthorized_bundle_->orchard_digest();
}

std::unique_ptr<AuthorizedOrchardBundle>
UnauthorizedOrchardBundleImpl::Complete(
    const std::array<uint8_t, kZCashDigestSize>& sighash) {
  auto authorized_orchard_bundle_result =
      orchard_unauthorized_bundle_->complete(sighash);
  if (!authorized_orchard_bundle_result->is_ok()) {
    return nullptr;
  }
  return base::WrapUnique<AuthorizedOrchardBundle>(
      new AuthorizedOrchardBundleImpl(
          authorized_orchard_bundle_result->unwrap()));
}

}  // namespace brave_wallet::orchard
