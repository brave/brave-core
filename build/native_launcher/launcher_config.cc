/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/native_launcher/launcher_config.h"

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/values.h"

namespace {

base::FilePath::StringType ToNativeString(const std::string& utf8) {
  return base::FilePath::FromUTF8Unsafe(utf8).value();
}

}  // namespace

base::expected<LauncherConfig, std::string> ParseLauncherConfig(
    const base::FilePath& json_path) {
  std::string json_data;
  if (!base::ReadFileToString(json_path, &json_data)) {
    return base::unexpected(base::StrCat(
        {"Failed to read sidecar config at: ", json_path.AsUTF8Unsafe()}));
  }

  const std::optional<base::Value> parsed_json =
      base::JSONReader::Read(json_data, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed_json || !parsed_json->is_dict()) {
    return base::unexpected("Invalid JSON in sidecar config.");
  }

  const base::DictValue& dict = parsed_json->GetDict();

  const std::string* executable = dict.FindString("executable");
  if (!executable) {
    return base::unexpected("Config missing required 'executable' string.");
  }

  LauncherConfig config;
  config.executable = ToNativeString(*executable);

  if (const base::Value* args_value = dict.Find("args")) {
    if (!args_value->is_list()) {
      return base::unexpected("Config 'args' must be a list of strings.");
    }
    for (const auto& arg : args_value->GetList()) {
      if (!arg.is_string()) {
        return base::unexpected("Config 'args' entries must be strings.");
      }
      config.args.push_back(ToNativeString(arg.GetString()));
    }
  }

  if (const base::Value* env_value = dict.Find("env")) {
    if (!env_value->is_dict()) {
      return base::unexpected(
          "Config 'env' must be an object of string values.");
    }
    for (auto [key, val] : env_value->GetDict()) {
      if (!val.is_string()) {
        return base::unexpected(base::StrCat(
            {"Config 'env' value for '", key, "' must be a string."}));
      }
      config.env[ToNativeString(key)] = ToNativeString(val.GetString());
    }
  }

  if (const base::Value* cwd_value = dict.Find("cwd")) {
    if (!cwd_value->is_string()) {
      return base::unexpected("Config 'cwd' must be a string.");
    }
    config.cwd = ToNativeString(cwd_value->GetString());
  }

  return config;
}
