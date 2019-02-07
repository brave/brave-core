/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_CALLBACK_HANDLER_
#define BAT_LEDGER_LEDGER_CALLBACK_HANDLER_

#include <string>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT enum Result {
  LEDGER_OK = 0,
  LEDGER_ERROR = 1,
  NO_PUBLISHER_STATE = 2,
  NO_LEDGER_STATE = 3,
  INVALID_PUBLISHER_STATE = 4,
  INVALID_LEDGER_STATE = 5,
  CAPTCHA_FAILED = 6,
  NO_PUBLISHER_LIST = 7,

  TOO_MANY_RESULTS = 8,
  NOT_FOUND = 9,

  REGISTRATION_VERIFICATION_FAILED = 10,
  BAD_REGISTRATION_RESPONSE = 11,
  WALLET_CREATED = 12,
  GRANT_NOT_FOUND = 13,

  AC_TABLE_EMPTY = 14,
  NOT_ENOUGH_FUNDS = 15,
  TIP_ERROR = 16,
  CORRUPTED_WALLET = 17,
  GRANT_ALREADY_CLAIMED = 18,
  // some more useful result codes should go here

  LEDGER_RESULT_END = 19,
};

// LedgerCallbackHandler must not be destroyed if they have pending callbacks
class LEDGER_EXPORT LedgerCallbackHandler {
 public:
  virtual ~LedgerCallbackHandler() = default;

  virtual void OnLedgerStateLoaded(Result result,
                                   const std::string& data) {};
  virtual void OnLedgerStateSaved(Result result) {};

  virtual void OnPublisherStateLoaded(Result result,
                                      const std::string& data) {};
  virtual void OnPublisherStateSaved(Result result) {};
  virtual void OnPublishersListSaved(Result result) {};

  virtual void OnPublisherListLoaded(Result result,
                                      const std::string& data) {};
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CALLBACK_HANDLER_
