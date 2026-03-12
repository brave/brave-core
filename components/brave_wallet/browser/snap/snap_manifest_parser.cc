/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_manifest_parser.h"

#include "base/json/json_reader.h"

namespace brave_wallet {

SnapManifestParser::Result::Result() = default;
SnapManifestParser::Result::Result(Result&&) = default;
SnapManifestParser::Result::~Result() = default;

namespace {

constexpr const char* kAllowedPermissions[] = {
    "snap_getBip44Entropy",
    "snap_getBip32Entropy",
    "snap_dialog",
    "snap_confirm",
    "snap_notify",
    "snap_manageState",
    "endowment:network-access",
    "endowment:rpc",
    "endowment:webassembly",
    "endowment:page-home",
    "endowment:lifecycle-hooks",
    "endowment:cronjob",
    "endowment:transaction-insight",
    "endowment:signature-insight",
    "endowment:ethereum-provider",
};

bool ValidatePermissions(const base::Value::Dict& permissions,
                         std::vector<std::string>& out_permissions,
                         std::string& out_disallowed) {
  for (const auto [key, _] : permissions) {
    bool allowed = false;
    for (const char* p : kAllowedPermissions) {
      if (key == p) {
        allowed = true;
        break;
      }
    }
    if (!allowed) {
      out_disallowed = key;
      return false;
    }
    out_permissions.push_back(key);
  }
  return true;
}

// Reads endowment:rpc fields from |initial_perms| into |manifest|.
void ParseRpcEndowment(const base::Value::Dict& initial_perms,
                       mojom::SnapManifest& manifest) {
  const base::Value* rpc_val = initial_perms.Find("endowment:rpc");
  if (!rpc_val || !rpc_val->is_dict()) {
    return;
  }
  const base::Value::Dict& rpc = rpc_val->GetDict();
  manifest.allow_dapps = rpc.FindBool("dapps").value_or(false);
  manifest.allow_snaps = rpc.FindBool("snaps").value_or(false);
  if (const base::Value::List* origins = rpc.FindList("allowedOrigins")) {
    for (const auto& item : *origins) {
      if (item.is_string()) {
        manifest.allowed_rpc_origins.push_back(item.GetString());
      }
    }
  }
}

}  // namespace

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
  const base::Value::Dict& dict = parsed->GetDict();

  if (const std::string* name = dict.FindString("proposedName")) {
    result.manifest->proposed_name = *name;
  } else {
    result.manifest->proposed_name = snap_id;
  }

  if (const std::string* desc = dict.FindString("description")) {
    result.manifest->description = *desc;
  }

  const base::Value::Dict* source = dict.FindDict("source");
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

  const base::Value::Dict* initial_perms = dict.FindDict("initialPermissions");
  if (initial_perms) {
    std::string disallowed;
    if (!ValidatePermissions(*initial_perms, result.manifest->permissions,
                             disallowed)) {
      result.error = "Snap requests a disallowed permission: " + disallowed;
      result.manifest.reset();
      return result;
    }
    ParseRpcEndowment(*initial_perms, *result.manifest);
  }

  return result;
}

}  // namespace brave_wallet
