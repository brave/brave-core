/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_CLIENT_WEBREQUEST_H_
#define BRAVELEDGER_BAT_CLIENT_WEBREQUEST_H_


#include <vector>
#include <string>

#include "bat_helper.h"

namespace braveledger_bat_client_webrequest {

  class BatClientWebRequest
  {
  public:
    BatClientWebRequest() = default;

    void run(const std::string& url, braveledger_bat_helper::FetchCallback callback,
      const std::vector<std::string>& headers, const std::string& content,
      const std::string& contentType, const braveledger_bat_helper::FETCH_CALLBACK_EXTRA_DATA_ST& extraData,
      const braveledger_bat_helper::URL_METHOD& method) {

      //TODO: platform-dependent implementation 
    }
  };

} //namespace braveledger_bat_client_webrequest

#endif // BRAVELEDGER_BAT_CLIENT_WEBREQUEST_H_
