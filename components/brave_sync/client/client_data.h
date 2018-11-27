/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_DATA_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_DATA_H_

#include <string>

namespace brave_sync {
namespace client_data {

//typedef std::vector<unsigned char> Uint8Array;
//using Uint8Array = std::vector<unsigned char>;

class Config {
public:
  Config();
  // version of API
  std::string api_version;
  // url of the server
  std::string server_url;
  // whether sync lib produces debug messages
  bool debug;
};

} // namespace client_data
} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_DATA_H_
