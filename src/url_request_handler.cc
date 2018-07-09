/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "url_request_handler.h"

namespace bat_ledger {

URLRequestHandler::URLRequestHandler() {}

URLRequestHandler::~URLRequestHandler() {
  Clear();
}

void URLRequestHandler::Clear() {
  request_handlers_.clear();
}

void URLRequestHandler::OnURLRequestResponse(uint64_t request_id,
                                            int response_code,
                                            const std::string& response) {
  if (!RunRequestHandler(request_id, response_code == 200, response)) {
    LOG(ERROR) << "no request handler found for " << request_id;
    return;
  }
}

bool URLRequestHandler::AddRequestHandler(
    std::unique_ptr<ledger::LedgerURLLoader> loader,
    URLRequestCallback callback) {
  uint64_t request_id = loader->request_id();
  if (request_handlers_.find(request_id) != request_handlers_.end())
    return false;

  request_handlers_[request_id] = callback;
  loader->Start();
  return true;
}

bool URLRequestHandler::RunRequestHandler(uint64_t request_id,
                                          bool success,
                                          const std::string& response) {
  if (request_handlers_.find(request_id) == request_handlers_.end())
    return false;

  auto callback = request_handlers_[request_id];
  request_handlers_.erase(request_id);
  callback(success, response);
  return true;
}

}  // namespace bat_ledger
