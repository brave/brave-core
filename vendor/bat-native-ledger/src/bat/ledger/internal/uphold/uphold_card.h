/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_CARD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_CARD_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/uphold/uphold.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class UpholdServer;
}

namespace uphold {

const char kCardName[] = "Brave Browser";

class UpholdCard {
 public:
  explicit UpholdCard(LedgerImpl*);

  ~UpholdCard();

  void CreateBATCardIfNecessary(const std::string& access_token,
                                CreateCardCallback) const;

 private:
  void OnGetBATCardId(CreateCardCallback,
                      const std::string& access_token,
                      mojom::Result,
                      std::string&& id) const;

  void OnCreateBATCard(CreateCardCallback,
                       const std::string& access_token,
                       mojom::Result,
                       std::string&& id) const;

  void OnUpdateBATCardSettings(CreateCardCallback,
                               std::string&& id,
                               mojom::Result) const;

  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_CARD_H_
