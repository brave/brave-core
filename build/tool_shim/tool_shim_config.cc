/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/tool_shim/tool_shim_config.h"

#include <ostream>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/build/tool_shim/utils.h"

base::expected<ToolShimConfig, std::string> ParseToolShimConfig(
    const base::FilePath& json_path) {
  std::string json_text;
  if (!base::ReadFileToString(json_path, &json_text)) {
    return base::unexpected(
        base::StrCat({"Failed to read config at: ", json_path.AsUTF8Unsafe()}));
  }

  const base::expected<base::Value, base::JSONReader::Error> parsed_json =
      base::JSONReader::ReadAndReturnValueWithError(
          json_text, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed_json.has_value()) {
    return base::unexpected(
        base::StrCat({"Failed to parse config ", json_path.AsUTF8Unsafe(), " ",
                      parsed_json.error().ToString(), "\n", json_text}));
  }

  if (!parsed_json.value().is_dict()) {
    return base::unexpected(
        base::StrCat({"Config is not a dictionary:\n", json_text}));
  }

  const base::DictValue& dict = parsed_json->GetDict();

  const std::string* executable = dict.FindString("executable");
  if (!executable) {
    return base::unexpected(base::StrCat(
        {"Config missing required 'executable' string:\n", json_text}));
  }

  ToolShimConfig config;
  if (VLOG_IS_ON(1)) {
    config.json_path = json_path;
    config.json_text = json_text;
  }
  config.executable = ToNativeString(*executable);

  if (const base::Value* args_value = dict.Find("args")) {
    if (!args_value->is_list()) {
      return base::unexpected(base::StrCat(
          {"Config 'args' must be a list of strings:\n", json_text}));
    }
    for (const auto& arg : args_value->GetList()) {
      if (!arg.is_string()) {
        return base::unexpected(base::StrCat(
            {"Config 'args' entries must be strings:\n", json_text}));
      }
      config.args.push_back(ToNativeString(arg.GetString()));
    }
  }

  if (const base::Value* env_value = dict.Find("env")) {
    if (!env_value->is_dict()) {
      return base::unexpected(base::StrCat(
          {"Config 'env' must be an object of string values:\n", json_text}));
    }
    for (auto [key, val] : env_value->GetDict()) {
      if (!val.is_string()) {
        return base::unexpected(
            base::StrCat({"Config 'env' value for '", key,
                          "' must be a string:\n", json_text}));
      }
      config.env[ToNativeString(key)] = ToNativeString(val.GetString());
    }
  }

  if (const base::Value* cwd_value = dict.Find("cwd")) {
    if (!cwd_value->is_string()) {
      return base::unexpected(
          base::StrCat({"Config 'cwd' must be a string:\n", json_text}));
    }
    config.cwd = ToNativeString(cwd_value->GetString());
  }

  return config;
}

std::ostream& operator<<(std::ostream& os, const ToolShimConfig& config) {
  os << "Tool shim config:\n"
     << config.json_text << "\n at " << config.json_path;
  return os;
}
