/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/values.h"
#include "base/json/json_reader.h"
#include "brave/common/extensions/extension_constants.h"

namespace brave {

bool BraveRewriteManifest(const std::string& extension_id,
    base::DictionaryValue* manifest) {
  if (extension_id == ipfs_companion_extension_id ||
      extension_id == ipfs_companion_beta_extension_id) {
    // Set appropriate sockets permissions for IPFS Companion
    // extensions
    std::string json = R"(
      {
        "udp": {
          "send": "*",
          "bind": "*"
        },
        "tcp": {
          "connect": "*"
        },
        "tcpServer": {
          "listen": "*:*"
        }
      }
    )";

    base::Value sockets = base::JSONReader::Read(json).value();
    manifest->SetKey("sockets", std::move(sockets));
    return true;
  }
  return false;
}

}  // namespace brave

#include "../../../../extensions/browser/sandboxed_unpacker.cc"  // NOLINT
