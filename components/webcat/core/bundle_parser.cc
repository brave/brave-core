/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/bundle_parser.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace webcat {

namespace {

bool ParseManifestFromDict(const base::Value::Dict& dict, Manifest& manifest) {
  auto* app = dict.FindString("app");
  if (!app) return false;
  manifest.app = *app;

  auto* version = dict.FindString("version");
  if (!version) return false;
  manifest.version = *version;

  auto* default_csp = dict.FindString("default_csp");
  if (!default_csp) return false;
  manifest.default_csp = *default_csp;

  auto* default_index = dict.FindString("default_index");
  if (!default_index) return false;
  manifest.default_index = *default_index;

  auto* default_fallback = dict.FindString("default_fallback");
  if (!default_fallback) return false;
  manifest.default_fallback = *default_fallback;

  auto* files_dict = dict.FindDict("files");
  if (!files_dict) return false;
  for (auto [path, hash_val] : *files_dict) {
    auto* hash_str = hash_val.GetIfString();
    if (!hash_str) return false;
    manifest.files[path] = *hash_str;
  }

  auto* extra_csp_dict = dict.FindDict("extra_csp");
  if (extra_csp_dict) {
    for (auto [prefix, csp_val] : *extra_csp_dict) {
      auto* csp_str = csp_val.GetIfString();
      if (!csp_str) return false;
      manifest.extra_csp[prefix] = *csp_str;
    }
  }

  auto* wasm_list = dict.FindList("wasm");
  if (wasm_list) {
    for (const auto& entry : *wasm_list) {
      auto* hash_str = entry.GetIfString();
      if (!hash_str) return false;
      manifest.wasm.push_back(*hash_str);
    }
  }

  return true;
}

}  // namespace

ParseResult ParseBundle(const std::string& json_string) {
  ParseResult result;

  auto parsed = base::Value::FromJson(json_string);
  if (!parsed.has_value()) {
    result.error = WebcatError::kBundleParseError;
    result.error_detail = "Failed to parse JSON";
    return result;
  }

  auto* root = parsed->GetIfDict();
  if (!root) {
    result.error = WebcatError::kBundleParseError;
    result.error_detail = "Bundle root is not a JSON object";
    return result;
  }

  auto* manifest_dict = root->FindDict("manifest");
  if (!manifest_dict) {
    result.error = WebcatError::kBundleParseError;
    result.error_detail = "Missing 'manifest' object in bundle";
    return result;
  }

  auto* signatures_dict = root->FindDict("signatures");
  if (!signatures_dict) {
    result.error = WebcatError::kBundleParseError;
    result.error_detail = "Missing 'signatures' object in bundle";
    return result;
  }

  Bundle bundle;
  if (!ParseManifestFromDict(*manifest_dict, bundle.manifest)) {
    result.error = WebcatError::kBundleParseError;
    result.error_detail = "Failed to parse manifest fields";
    return result;
  }

  for (auto [pubkey, sig_val] : *signatures_dict) {
    auto* sig_str = sig_val.GetIfString();
    if (!sig_str) {
      result.error = WebcatError::kBundleParseError;
      result.error_detail = "Signature value is not a string";
      return result;
    }
    bundle.signatures[pubkey] = *sig_str;
  }

  std::string validation_error;
  if (!ValidateManifestStructure(bundle.manifest, validation_error)) {
    result.error = WebcatError::kManifestStructureInvalid;
    result.error_detail = validation_error;
    return result;
  }

  result.bundle = std::move(bundle);
  return result;
}

bool ValidateManifestStructure(const Manifest& manifest,
                               std::string& error_detail) {
  if (manifest.files.empty()) {
    error_detail = "Manifest 'files' map is empty";
    return false;
  }

  if (manifest.default_csp.empty()) {
    error_detail = "Manifest 'default_csp' is empty";
    return false;
  }

  if (manifest.default_index.empty()) {
    error_detail = "Manifest 'default_index' is empty";
    return false;
  }

  if (manifest.default_fallback.empty()) {
    error_detail = "Manifest 'default_fallback' is empty";
    return false;
  }

  if (!manifest.files.contains(manifest.default_index)) {
    error_detail = "Manifest 'default_index' not found in 'files' map";
    return false;
  }

  if (!manifest.files.contains(manifest.default_fallback)) {
    error_detail = "Manifest 'default_fallback' not found in 'files' map";
    return false;
  }

  for (const auto& [path, hash] : manifest.files) {
    if (path.empty()) {
      error_detail = "Manifest contains empty file path";
      return false;
    }
    if (hash.empty()) {
      error_detail = "Manifest contains empty hash for path: " + path;
      return false;
    }
    if (!path.starts_with("/")) {
      error_detail = "Manifest file path does not start with '/': " + path;
      return false;
    }
  }

  for (const auto& [prefix, csp] : manifest.extra_csp) {
    if (prefix.empty() || csp.empty()) {
      error_detail = "Manifest extra_csp contains empty key or value";
      return false;
    }
  }

  return true;
}

}  // namespace webcat
