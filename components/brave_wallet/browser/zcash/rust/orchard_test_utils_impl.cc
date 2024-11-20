// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_test_utils_impl.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"

namespace brave_wallet::orchard {

OrchardTestUtilsImpl::OrchardTestUtilsImpl() = default;

OrchardTestUtilsImpl::~OrchardTestUtilsImpl() = default;

OrchardCommitmentValue OrchardTestUtilsImpl::CreateMockCommitmentValue(
    uint32_t position,
    uint32_t rseed) {
  return create_mock_commitment(position, rseed);
}

// static
std::unique_ptr<OrchardTestUtils> OrchardTestUtils::Create() {
  return std::make_unique<OrchardTestUtilsImpl>();
}

}  // namespace brave_wallet::orchard
