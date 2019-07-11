/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_extension_converter.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/common/network_constants.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "chrome/common/chrome_paths.h"
#include "crypto/sha2.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "url/gurl.h"

using extensions::Extension;
using extensions::Manifest;

namespace greaselion {

scoped_refptr<Extension> ConvertGreaselionRuleToExtensionOnTaskRunner(
    const GreaselionRule& rule,
    const base::FilePath& extensions_dir) {
  base::FilePath install_temp_dir =
      extensions::file_util::GetInstallTempDir(extensions_dir);
  if (install_temp_dir.empty()) {
    LOG(ERROR) << "Could not get path to profile temp directory";
    return NULL;
  }

  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDirUnderPath(install_temp_dir)) {
    LOG(ERROR) << "Could not create Greaselion temp directory";
    return NULL;
  }

  // Create the manifest
  std::unique_ptr<base::DictionaryValue> root(new base::DictionaryValue);

  // Create the public key.
  // Greaselion scripts are not signed, but the public key for an extension
  // doubles as its unique identity, and we need one of those, so we add the
  // rule name to a known Brave domain and hash the result to create a public
  // key.
  char raw[crypto::kSHA256Length] = {0};
  std::string key;
  std::string script_name = rule.name();
  crypto::SHA256HashString(kBraveUpdatesExtensionsEndpoint + script_name, raw,
                           crypto::kSHA256Length);
  base::Base64Encode(base::StringPiece(raw, crypto::kSHA256Length), &key);

  root->SetString(extensions::manifest_keys::kName, script_name);
  root->SetString(extensions::manifest_keys::kVersion, "1.0");
  root->SetString(extensions::manifest_keys::kDescription, "");
  root->SetString(extensions::manifest_keys::kPublicKey, key);

  auto js_files = std::make_unique<base::ListValue>();
  for (auto script : rule.scripts())
    js_files->AppendString(script.BaseName().value());

  auto matches = std::make_unique<base::ListValue>();
  for (auto url_pattern : rule.url_patterns())
    matches->AppendString(url_pattern);

  auto content_script = std::make_unique<base::DictionaryValue>();
  content_script->Set(extensions::manifest_keys::kMatches, std::move(matches));
  content_script->Set(extensions::manifest_keys::kJs, std::move(js_files));
  // All Greaselion scripts run at document start.
  content_script->SetString(extensions::manifest_keys::kRunAt,
                            extensions::manifest_values::kRunAtDocumentStart);

  auto content_scripts = std::make_unique<base::ListValue>();
  content_scripts->Append(std::move(content_script));

  root->Set(extensions::manifest_keys::kContentScripts,
            std::move(content_scripts));

  base::FilePath manifest_path =
      temp_dir.GetPath().Append(extensions::kManifestFilename);
  JSONFileValueSerializer serializer(manifest_path);
  if (!serializer.Serialize(*root)) {
    LOG(ERROR) << "Could not write Greaselion manifest";
    return NULL;
  }

  // Copy the script files to our extension directory.
  for (auto script : rule.scripts()) {
    if (!base::CopyFile(script, temp_dir.GetPath().Append(script.BaseName()))) {
      LOG(ERROR) << "Could not copy Greaselion script";
      return NULL;
    }
  }

  std::string error;
  scoped_refptr<Extension> extension =
      Extension::Create(temp_dir.GetPath(), Manifest::COMPONENT, *root,
                        Extension::NO_FLAGS, &error);
  if (!extension.get()) {
    LOG(ERROR) << "Could not create Greaselion extension";
    return NULL;
  }

  temp_dir.Take();  // The caller takes ownership of the directory.
  return extension;
}

}  // namespace greaselion
