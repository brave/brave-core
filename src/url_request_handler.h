/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_URL_REQUEST_HANDLER_H_
#define BAT_LEDGER_URL_REQUEST_HANDLER_H_

#include <functional>
#include <map>
#include <string>

#include "bat/ledger/ledger_callback_handler.h"
#include "bat_helper.h"

namespace bat_ledger {

class URLRequestHandler : public ledger::LedgerCallbackHandler {
 public:
  using URLRequestCallback = std::function<void (bool, const std::string&)>;

  URLRequestHandler();
  ~URLRequestHandler() override;

  void Clear();
  bool AddRequestHandler(uint64_t request_id, URLRequestCallback callback);
  bool RunRequestHandler(uint64_t request_id,
                         bool success,
                         const std::string& response);

 private:
  //  LedgerCallbackHandler impl
  void OnURLRequestResponse(uint64_t request_id,
                            int response_code,
                            const std::string& response) override;

  std::map<int, URLRequestCallback> request_handlers_;
 };
}  // namespace bat_ledger

#endif  // BAT_LEDGER_URL_REQUEST_HANDLER_H_
