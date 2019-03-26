/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/extensions/manifest_handlers/brave_manifest_override.h"

#include "base/json/json_reader.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "extensions/common/api/sockets/sockets_manifest_data.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

BraveOverridesHandler::BraveOverridesHandler() {
}

BraveOverridesHandler::~BraveOverridesHandler() {
}

bool BraveOverridesHandler::Parse(Extension* extension, base::string16* error) {
  bool result = ExtensionActionHandler::Parse(extension, error);
  if (extension) {
    if (extension->id() == pdfjs_extension_id) {
      // We want PDFJS, but we don't want the PDFJS page action
      ActionInfo::SetPageActionInfo(extension, nullptr);
    } else if (extension->id() == ipfs_companion_extension_id ||
               extension->id() == ipfs_companion_beta_extension_id) {
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
      std::unique_ptr<base::Value> sockets = base::JSONReader::Read(json);
      DCHECK(sockets);
      std::unique_ptr<SocketsManifestData> data =
          SocketsManifestData::FromValue(*sockets, error);
      if (!data)
        return false;

      extension->SetManifestData(manifest_keys::kSockets, std::move(data));
      return true;
    }
  }
  return result;
}

}  // namespace extensions
