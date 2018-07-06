/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_CALLBACK_HANDLER_
#define BAT_LEDGER_LEDGER_CALLBACK_HANDLER_

#include <string>

namespace ledger {

enum Result {
  OK = 0,
  ERROR = 1,
  // some more useful result codes should go here
};

// LedgerCallbackHandler must not be destroyed if they have pending callbacks
class LedgerCallbackHandler {
 public:
  virtual ~LedgerCallbackHandler() = default;

  virtual void OnLedgerStateLoaded(Result result,
                                   const std::string& data) {};
  virtual void OnPublisherStateLoaded(Result result,
                                      const std::string& data) {};
  virtual void OnPublisherStateSaved(Result result) {};
  virtual void OnLedgerStateSaved(Result result) {};
  virtual void OnURLRequestResponse(uint64_t request_id,
                                    int response_code,
                                    const std::string& response) {};
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_CALLBACK_HANDLER_
