/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_manifest_parser.h"

#include "base/json/json_reader.h"
#include "brave/components/brave_wallet/browser/snap/snap_manifest_helpers.h"

namespace brave_wallet {

SnapManifestParser::Result::Result() = default;
SnapManifestParser::Result::Result(Result&&) = default;
SnapManifestParser::Result::~Result() = default;

// static
SnapManifestParser::Result SnapManifestParser::Parse(
    const std::string& manifest_json,
    const std::string& snap_id,
    const std::string& version) {
  Result result;
  result.manifest = mojom::SnapManifest::New();

  std::optional<base::Value> parsed =
      base::JSONReader::Read(manifest_json, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    result.error = "Invalid snap.manifest.json";
    result.manifest.reset();
    return result;
  }
  const base::DictValue& dict = parsed->GetDict();

  if (const std::string* name = dict.FindString("proposedName")) {
    result.manifest->proposed_name = *name;
  } else {
    result.manifest->proposed_name = snap_id;
  }

  if (const std::string* desc = dict.FindString("description")) {
    result.manifest->description = *desc;
  }

  const base::DictValue* source = dict.FindDict("source");
  if (!source) {
    result.error = "Missing 'source' in snap.manifest.json";
    result.manifest.reset();
    return result;
  }
  const std::string* shasum = source->FindString("shasum");
  if (!shasum) {
    result.error = "Missing 'source.shasum' in snap.manifest.json";
    result.manifest.reset();
    return result;
  }
  result.expected_shasum = *shasum;

  const base::DictValue* initial_perms = dict.FindDict("initialPermissions");
  if (initial_perms) {
    for (const auto [key, value] : *initial_perms) {
      if (!IsAllowedSnapPermission(key)) {
        result.error = "Snap requests a disallowed permission: " + key;
        result.manifest.reset();
        return result;
      }
      ApplySnapPermissionFromValue(key, value, *result.manifest);
    }
  }

  return result;
}

}  // namespace brave_wallet
