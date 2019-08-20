/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_CALLBACK_HANDLER_
#define BAT_LEDGER_LEDGER_CALLBACK_HANDLER_

#include <string>

#include "bat/ledger/export.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using Result = ledger::mojom::Result;
using InitializeCallback = std::function<void(Result)>;

// LedgerCallbackHandler must not be destroyed if they have pending callbacks
class LEDGER_EXPORT LedgerCallbackHandler {
 public:
  virtual ~LedgerCallbackHandler() = default;

  virtual void OnLedgerStateLoaded(Result result,
      const std::string& data,
      InitializeCallback callback) {}

  virtual void OnLedgerStateSaved(Result result) {}

  virtual void OnPublisherStateLoaded(Result result,
      const std::string& data,
      InitializeCallback callback) {}

  virtual void OnPublisherStateSaved(Result result) {}

  virtual void OnPublishersListSaved(Result result) {}

  virtual void OnPublisherListLoaded(Result result, const std::string& data) {}
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CALLBACK_HANDLER_
