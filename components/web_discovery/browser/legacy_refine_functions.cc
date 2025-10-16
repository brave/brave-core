/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/legacy_refine_functions.h"

#include <optional>
#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/util.h"
#include "url/gurl.h"

namespace web_discovery {

namespace {

constexpr char kRefineSplitFuncId[] = "splitF";
constexpr char kRefineMaskURLFuncId[] = "maskU";
constexpr char kRefineParseURLFuncId[] = "parseU";
constexpr char kRefineJsonExtractFuncId[] = "json";

constexpr char kParseURLQueryExtractType[] = "qs";

std::string RefineSplit(const std::string& value,
                        const std::string& delimiter,
                        int index) {
  auto split = base::SplitStringUsingSubstr(
      value, delimiter, base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SPLIT_WANT_ALL);
  std::string encoded_result;
  if (index < 0 || static_cast<size_t>(index) >= split.size()) {
    encoded_result = value;
  } else {
    encoded_result = split[index];
  }
  return DecodeURLComponent(encoded_result);
}

std::optional<std::string> RefineParseURL(const std::string& value,
                                          const std::string& extract_type,
                                          const std::string& key) {
  if (extract_type != kParseURLQueryExtractType) {
    return std::nullopt;
  }
  GURL url(value);
  if (!url.is_valid() || !url.has_query()) {
    return std::nullopt;
  }
  auto query_value = ExtractValueFromQueryString(url.query_piece(), key);
  return query_value;
}

std::optional<std::string> RefineJsonExtract(const std::string& value,
                                             const std::string& path,
                                             bool extract_objects) {
  auto parsed =
      base::JSONReader::ReadDict(value, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed) {
    return std::nullopt;
  }
  const auto* found_value = parsed->FindByDottedPath(path);
  if (!found_value) {
    return std::nullopt;
  }
  if (found_value->is_string()) {
    return found_value->GetString();
  }
  if ((found_value->is_dict() || found_value->is_list()) && !extract_objects) {
    return std::nullopt;
  }
  std::string encoded_value;
  if (!base::JSONWriter::Write(*found_value, &encoded_value)) {
    return std::nullopt;
  }
  return encoded_value;
}

}  // namespace

std::optional<std::string> ExecuteRefineFunctions(
    const RefineFunctionList& function_list,
    std::string value) {
  std::optional<std::string> result = value;
  for (const auto& function_args : function_list) {
    if (function_args.empty()) {
      continue;
    }
    const auto& func_name = function_args[0];
    if (func_name == kRefineSplitFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_int()) {
        result = RefineSplit(*result, function_args[1].GetString(),
                             function_args[2].GetInt());
      }
    } else if (func_name == kRefineMaskURLFuncId) {
      result = MaskURL(GURL(value), false);
    } else if (func_name == kRefineParseURLFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_string()) {
        result = RefineParseURL(*result, function_args[1].GetString(),
                                function_args[2].GetString());
      }
    } else if (func_name == kRefineJsonExtractFuncId) {
      if (function_args.size() >= 3 && function_args[1].is_string() &&
          function_args[2].is_bool()) {
        result = RefineJsonExtract(*result, function_args[1].GetString(),
                                   function_args[2].GetBool());
      }
    }
    if (!result) {
      break;
    }
  }
  return result;
}

}  // namespace web_discovery
