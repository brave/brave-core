/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_manifest_parser.h"

#include <string_view>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"

namespace brave_wallet {

SnapManifestParser::Result::Result() = default;
SnapManifestParser::Result::Result(Result&&) = default;
SnapManifestParser::Result::~Result() = default;

namespace {

constexpr const char* kAllowedPermissions[] = {
    "snap_getBip44Entropy",
    "snap_getBip32Entropy",
    "snap_getEntropy",
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
    "endowment:name-lookup",
};

bool ValidatePermissions(const base::DictValue& permissions,
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
void ParseRpcEndowment(const base::DictValue& initial_perms,
                       mojom::SnapManifest& manifest) {
  const base::Value* rpc_val = initial_perms.Find("endowment:rpc");
  if (!rpc_val || !rpc_val->is_dict()) {
    return;
  }
  const base::DictValue& rpc = rpc_val->GetDict();
  manifest.allow_dapps = rpc.FindBool("dapps").value_or(false);
  manifest.allow_snaps = rpc.FindBool("snaps").value_or(false);
  if (const base::ListValue* origins = rpc.FindList("allowedOrigins")) {
    for (const auto& item : *origins) {
      if (item.is_string()) {
        manifest.allowed_rpc_origins.push_back(item.GetString());
      }
    }
  }
}

// Appends the string elements of |list_name| within |dict| to |out|.
void ReadStringList(const base::DictValue& dict,
                    std::string_view list_name,
                    std::vector<std::string>& out) {
  const base::ListValue* list = dict.FindList(list_name);
  if (!list) {
    return;
  }
  for (const auto& item : *list) {
    if (item.is_string()) {
      out.push_back(item.GetString());
    }
  }
}

// Reads endowment:name-lookup fields from |initial_perms| into |manifest|.
// https://docs.metamask.io/snaps/reference/permissions/#endowmentname-lookup
// The config object and all of its fields are optional.
void ParseNameLookupEndowment(const base::DictValue& initial_perms,
                              mojom::SnapManifest& manifest) {
  const base::Value* val = initial_perms.Find("endowment:name-lookup");
  if (!val || !val->is_dict()) {
    return;
  }
  const base::DictValue& cfg = val->GetDict();
  ReadStringList(cfg, "chains", manifest.name_lookup_chains);
  if (const base::DictValue* matchers = cfg.FindDict("matchers")) {
    ReadStringList(*matchers, "tlds", manifest.name_lookup_tlds);
    ReadStringList(*matchers, "schemes", manifest.name_lookup_schemes);
  }
}

}  // namespace

// static
SnapManifestParser::Result SnapManifestParser::Parse(
    const std::string& manifest_json,
    const std::string& snap_id,
    const std::string& version) {
  LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse snap_id=" << snap_id
             << " version=" << version << " json_size=" << manifest_json.size();
  Result result;
  result.manifest = mojom::SnapManifest::New();

  std::optional<base::Value> parsed =
      base::JSONReader::Read(manifest_json, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: JSON parse FAILED";
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
  LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: proposedName='"
             << result.manifest->proposed_name << "'";

  if (const std::string* desc = dict.FindString("description")) {
    result.manifest->description = *desc;
  }
  LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: description='"
             << result.manifest->description << "'";

  const base::DictValue* source = dict.FindDict("source");
  if (!source) {
    LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: missing 'source'";
    result.error = "Missing 'source' in snap.manifest.json";
    result.manifest.reset();
    return result;
  }
  const std::string* shasum = source->FindString("shasum");
  if (!shasum) {
    LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: missing 'source.shasum'";
    result.error = "Missing 'source.shasum' in snap.manifest.json";
    result.manifest.reset();
    return result;
  }
  result.expected_shasum = *shasum;
  LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: expected_shasum='"
             << result.expected_shasum << "'";

  const base::DictValue* initial_perms = dict.FindDict("initialPermissions");
  LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: initialPermissions present="
             << (initial_perms != nullptr);
  if (initial_perms) {
    std::string disallowed;
    if (!ValidatePermissions(*initial_perms, result.manifest->permissions,
                             disallowed)) {
      LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: disallowed permission='"
                 << disallowed << "'";
      result.error = "Snap requests a disallowed permission: " + disallowed;
      result.manifest.reset();
      return result;
    }
    LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: permissions count="
               << result.manifest->permissions.size();
    ParseRpcEndowment(*initial_perms, *result.manifest);
    LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: allow_dapps="
               << result.manifest->allow_dapps
               << " allow_snaps=" << result.manifest->allow_snaps;
    ParseNameLookupEndowment(*initial_perms, *result.manifest);
  }

  LOG(ERROR) << "XXXZZZ SnapManifestParser::Parse: done, no error";
  return result;
}

}  // namespace brave_wallet
