/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_MANIFEST_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_MANIFEST_PARSER_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// Parses and validates a snap.manifest.json string.
// All manifest-specific knowledge lives here: the permissions allowlist,
// endowment:rpc deserialization, and field extraction.
// On success, |manifest| is a populated mojom::SnapManifestPtr.
// On failure, |error| is non-empty and |manifest| may be null.
class SnapManifestParser {
 public:
  struct Result {
    Result();
    Result(Result&&);
    ~Result();

    mojom::SnapManifestPtr manifest;
    std::string expected_shasum;
    std::string error;  // non-empty on any parse or validation failure
  };

  // Parses |manifest_json| and writes into the returned Result.
  // |snap_id| is stored separately (not in SnapManifest) but is validated.
  static Result Parse(const std::string& manifest_json,
                      const std::string& snap_id,
                      const std::string& version);

 private:
  SnapManifestParser() = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_MANIFEST_PARSER_H_
