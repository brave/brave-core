/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_WEBREQUEST_H_
#define BRAVELEDGER_BAT_CLIENT_WEBREQUEST_H_

#include <vector>
#include <string>

// TODO(bridiver) - hacky temp workaround
#if defined CHROMIUM_BUILD
#include "base/callback.h"
#endif

namespace braveledger_bat_helper {
struct FETCH_CALLBACK_EXTRA_DATA_ST;
}

namespace braveledger_bat_client_webrequest {

enum URL_METHOD {
  GET = 0,
  PUT = 1,
  POST = 2
};

using FetchCallbackSignature = void (bool, const std::string&, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST&);
// TODO(bridiver) - another hacky temp workaround
#if !defined CHROMIUM_BUILD
using FetchCallback = std::function<FetchCallbackSignature>;
#else
using FetchCallback = base::Callback<FetchCallbackSignature>;
#endif

// platform-dependent implementation
class BatClientWebRequest {
 public:
  BatClientWebRequest() = default;
  virtual void run(const std::string& url, FetchCallback callback,
      const std::vector<std::string>& headers, const std::string& content,
      const std::string& contentType, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData,
      const URL_METHOD& method) = 0;
  virtual void Stop() = 0;
  virtual void Start() = 0;
};

}  // namespace braveledger_bat_client_webrequest

#endif // BRAVELEDGER_BAT_CLIENT_WEBREQUEST_H_
