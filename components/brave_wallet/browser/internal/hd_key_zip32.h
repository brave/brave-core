// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ZIP32_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ZIP32_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/zcash/rs/lib.rs.h"
#endif

namespace brave_wallet {

enum class OrchardKind { External, Internal };

class HDKeyZip32 {
 public:
#if BUILDFLAG(ENABLE_ORCHARD)
  explicit HDKeyZip32(rust::Box<zcash::OrchardExtendedSpendingKeyResult> esk);
#else
  HDKeyZip32();
#endif

  ~HDKeyZip32();

  static std::unique_ptr<HDKeyZip32> GenerateFromSeed(
      const std::vector<uint8_t>& seed);

  std::unique_ptr<HDKeyZip32> DeriveHardenedChild(uint32_t index);

  std::optional<std::array<uint8_t, kOrchardRawBytesSize>>
  GetDiversifiedAddress(uint32_t div_index, OrchardKind kind);

 private:
#if BUILDFLAG(ENABLE_ORCHARD)
  rust::Box<zcash::OrchardExtendedSpendingKeyResult> extended_spending_key_;
#endif
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_HD_KEY_ZIP32_H_
