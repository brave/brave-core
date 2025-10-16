/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/value_transform.h"

#include <string>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/util.h"
#include "url/gurl.h"

namespace web_discovery {

namespace {

std::optional<base::flat_set<std::string>> ListToStringSet(
    const base::Value::List& list) {
  base::flat_set<std::string> result;
  for (const auto& item : list) {
    if (!item.is_string()) {
      return std::nullopt;
    }
    result.insert(item.GetString());
  }
  return result;
}

// Transform function names
constexpr std::string_view kTrySplit = "trySplit";
constexpr std::string_view kDecodeURIComponent = "decodeURIComponent";
constexpr std::string_view kFilterExact = "filterExact";
constexpr std::string_view kTryDecodeURIComponent = "tryDecodeURIComponent";
constexpr std::string_view kRemoveParams = "removeParams";
constexpr std::string_view kMaskU = "maskU";
constexpr std::string_view kSplit = "split";
constexpr std::string_view kTrim = "trim";
constexpr std::string_view kRelaxedMaskU = "relaxedMaskU";
constexpr std::string_view kJson = "json";
constexpr std::string_view kQueryParam = "queryParam";
constexpr std::string_view kRequireURL = "requireURL";

class DecodeURIComponentTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition,
      bool try_mode) {
    // transform_definition should be ["decodeURIComponent"] or
    // ["tryDecodeURIComponent"]
    if (transform_definition.size() != 1) {
      return nullptr;
    }

    return std::make_unique<DecodeURIComponentTransform>(try_mode);
  }

  explicit DecodeURIComponentTransform(bool try_mode) : try_mode_(try_mode) {}

  std::optional<std::string> Process(std::string_view input) override {
    std::string output = DecodeURLComponent(input);
    // Count '%25' sequences in input (these should decode to '%' in output)
    size_t input_percent25_count = 0;
    size_t pos = 0;
    while ((pos = input.find("%25", pos)) != std::string_view::npos) {
      input_percent25_count++;
      pos += 3;  // Move past "%25"
    }

    // Count '%' characters in output
    size_t output_percent_count = std::count(output.begin(), output.end(), '%');

    // If output has more '%' than expected from '%25' decoding, decode failed
    // `DecodeURLComponent` will output invalid escape sequences, which will
    // result in extra '%' characters in the output.
    bool decode_failed = output_percent_count > input_percent25_count;

    if (decode_failed) {
      return try_mode_ ? std::optional<std::string>(input) : std::nullopt;
    }

    return output;
  }

 private:
  bool try_mode_;
};

class FilterExactTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition) {
    // transform_definition should be ["filterExact", [allowed_strings]]
    if (transform_definition.size() != 2 ||
        !transform_definition[1].is_list()) {
      return nullptr;
    }

    auto allowed_strings = ListToStringSet(transform_definition[1].GetList());
    if (!allowed_strings) {
      return nullptr;
    }

    return std::make_unique<FilterExactTransform>(std::move(*allowed_strings));
  }

  explicit FilterExactTransform(base::flat_set<std::string>&& allowed_strings)
      : allowed_strings_(std::move(allowed_strings)) {}

  std::optional<std::string> Process(std::string_view input) override {
    return allowed_strings_.contains(input) ? std::optional<std::string>(input)
                                            : std::nullopt;
  }

 private:
  base::flat_set<std::string> allowed_strings_;
};

class RemoveParamsTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition) {
    // transform_definition should be ["removeParams", [params_to_remove]]
    if (transform_definition.size() != 2 ||
        !transform_definition[1].is_list()) {
      return nullptr;
    }

    auto params_to_remove = ListToStringSet(transform_definition[1].GetList());
    if (!params_to_remove) {
      return nullptr;
    }

    return std::make_unique<RemoveParamsTransform>(
        std::move(*params_to_remove));
  }

  explicit RemoveParamsTransform(base::flat_set<std::string>&& params_to_remove)
      : params_to_remove_(std::move(params_to_remove)) {}

  std::optional<std::string> Process(std::string_view input) override {
    GURL url(input);
    if (!url.is_valid()) {
      return std::nullopt;
    }

    if (!url.has_query()) {
      return std::string(input);
    }

    // Parse query parameters and filter out unwanted ones
    auto query_pairs =
        base::SplitStringPiece(url.query_piece(), "&", base::TRIM_WHITESPACE,
                               base::SPLIT_WANT_NONEMPTY);

    std::vector<std::string_view> kept_params;
    for (const auto& pair : query_pairs) {
      auto key_value = base::SplitStringPiece(pair, "=", base::TRIM_WHITESPACE,
                                              base::SPLIT_WANT_NONEMPTY);

      if (key_value.empty()) {
        continue;
      }

      if (!params_to_remove_.contains(key_value[0])) {
        kept_params.push_back(pair);
      }
    }

    // Use GURL::Replacements to modify the clone
    GURL::Replacements replacements;
    std::string new_query;
    if (kept_params.empty()) {
      replacements.ClearQuery();
    } else {
      new_query = base::JoinString(kept_params, "&");
      replacements.SetQueryStr(new_query);
    }

    auto result_url = url.ReplaceComponents(replacements);
    if (!result_url.is_valid()) {
      return std::nullopt;
    }
    return result_url.spec();
  }

 private:
  base::flat_set<std::string> params_to_remove_;
};

class MaskUTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition,
      bool relaxed = false) {
    // transform_definition should be ["maskU"] or ["relaxedMaskU"]
    if (transform_definition.size() != 1) {
      return nullptr;
    }

    return std::make_unique<MaskUTransform>(relaxed);
  }

  explicit MaskUTransform(bool relaxed = false) : relaxed_(relaxed) {}

  std::optional<std::string> Process(std::string_view input) override {
    GURL url(input);
    if (!url.is_valid()) {
      return std::nullopt;
    }
    return MaskURL(url, relaxed_);
  }

 private:
  bool relaxed_;
};

class SplitTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition,
      bool try_mode = false) {
    // transform_definition should be ["split", split_on, arr_pos] or
    // ["trySplit", split_on, arr_pos]
    if (transform_definition.size() != 3 ||
        !transform_definition[1].is_string() ||
        !transform_definition[2].is_int()) {
      return nullptr;
    }

    auto split_on = transform_definition[1].GetString();
    auto arr_pos = transform_definition[2].GetInt();
    if (arr_pos < 0 || split_on.empty()) {
      return nullptr;
    }

    return std::make_unique<SplitTransform>(
        split_on, static_cast<size_t>(arr_pos), try_mode);
  }

  SplitTransform(const std::string& split_on,
                 size_t arr_pos,
                 bool try_mode = false)
      : split_on_(split_on), arr_pos_(arr_pos), try_mode_(try_mode) {}

  std::optional<std::string> Process(std::string_view input) override {
    auto parts = base::SplitStringPieceUsingSubstr(
        input, split_on_, base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

    if (parts.size() == 1 && !try_mode_) {
      return std::nullopt;
    }

    if (arr_pos_ >= parts.size()) {
      return try_mode_ ? std::optional<std::string>(input) : std::nullopt;
    }

    return std::string(parts[arr_pos_]);
  }

 private:
  std::string split_on_;
  size_t arr_pos_;
  bool try_mode_;
};

class TrimTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition) {
    // transform_definition should be ["trim"]
    if (transform_definition.size() != 1) {
      return nullptr;
    }

    return std::make_unique<TrimTransform>();
  }

  TrimTransform() = default;

  std::optional<std::string> Process(std::string_view input) override {
    return std::string(base::TrimWhitespaceASCII(input, base::TRIM_ALL));
  }
};

class JsonTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition) {
    // transform_definition should be ["json", path] or ["json", path,
    // extract_objects]
    if (transform_definition.size() < 2 || transform_definition.size() > 3 ||
        !transform_definition[1].is_string()) {
      return nullptr;
    }

    std::string path = transform_definition[1].GetString();
    bool extract_objects = false;

    if (transform_definition.size() == 3) {
      if (!transform_definition[2].is_bool()) {
        return nullptr;
      }
      extract_objects = transform_definition[2].GetBool();
    }

    return std::make_unique<JsonTransform>(std::move(path), extract_objects);
  }

  JsonTransform(std::string path, bool extract_objects)
      : path_(std::move(path)), extract_objects_(extract_objects) {}

  std::optional<std::string> Process(std::string_view input) override {
    auto json_dict =
        base::JSONReader::ReadDict(input, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
    if (!json_dict.has_value()) {
      return "";  // Return empty string on parse error
    }

    // Use FindByDottedPath to navigate to the specified path
    const base::Value* current = json_dict->FindByDottedPath(path_);
    if (!current) {
      return "";
    }

    // Extract the value based on type
    switch (current->type()) {
      case base::Value::Type::STRING:
        return current->GetString();
      case base::Value::Type::INTEGER:
        return base::NumberToString(current->GetInt());
      case base::Value::Type::DOUBLE:
        return base::NumberToString(current->GetDouble());
      case base::Value::Type::BOOLEAN:
        return current->GetBool() ? "true" : "false";
      case base::Value::Type::DICT:
      case base::Value::Type::LIST:
        if (extract_objects_) {
          // Return JSON representation of objects/arrays if requested
          std::string json_output;
          if (base::JSONWriter::Write(*current, &json_output)) {
            return json_output;
          }
        }
        return "";  // Prevent uncontrolled text extraction
      default:
        return "";  // Prevent uncontrolled text extraction
    }
  }

 private:
  std::string path_;
  bool extract_objects_;
};

class QueryParamTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition) {
    // transform_definition should be ["queryParam", query_param]
    if (transform_definition.size() != 2 ||
        !transform_definition[1].is_string()) {
      return nullptr;
    }

    return std::make_unique<QueryParamTransform>(
        transform_definition[1].GetString());
  }

  explicit QueryParamTransform(std::string query_param)
      : query_param_(std::move(query_param)) {}

  std::optional<std::string> Process(std::string_view input) override {
    return ExtractValueFromQueryString(input, query_param_);
  }

 private:
  std::string query_param_;
};

class RequireURLTransform : public ValueTransform {
 public:
  static std::unique_ptr<ValueTransform> Create(
      const base::Value::List& transform_definition) {
    // transform_definition should be ["requireURL"]
    if (transform_definition.size() != 1) {
      return nullptr;
    }

    return std::make_unique<RequireURLTransform>();
  }

  RequireURLTransform() = default;

  std::optional<std::string> Process(std::string_view input) override {
    return GURL(input).is_valid() ? std::optional<std::string>(input)
                                  : std::nullopt;
  }
};

}  // namespace

// Factory function implementation
std::unique_ptr<ValueTransform> CreateValueTransform(
    const base::Value::List& transform_definition) {
  if (transform_definition.empty() || !transform_definition[0].is_string()) {
    return nullptr;
  }

  const std::string& transform_name = transform_definition[0].GetString();

  if (transform_name == kTrySplit) {
    return SplitTransform::Create(transform_definition, true);
  } else if (transform_name == kSplit) {
    return SplitTransform::Create(transform_definition, false);
  } else if (transform_name == kTryDecodeURIComponent) {
    return DecodeURIComponentTransform::Create(transform_definition, true);
  } else if (transform_name == kDecodeURIComponent) {
    return DecodeURIComponentTransform::Create(transform_definition, false);
  } else if (transform_name == kFilterExact) {
    return FilterExactTransform::Create(transform_definition);
  } else if (transform_name == kRemoveParams) {
    return RemoveParamsTransform::Create(transform_definition);
  } else if (transform_name == kMaskU) {
    return MaskUTransform::Create(transform_definition, false);
  } else if (transform_name == kRelaxedMaskU) {
    return MaskUTransform::Create(transform_definition, true);
  } else if (transform_name == kTrim) {
    return TrimTransform::Create(transform_definition);
  } else if (transform_name == kJson) {
    return JsonTransform::Create(transform_definition);
  } else if (transform_name == kQueryParam) {
    return QueryParamTransform::Create(transform_definition);
  } else if (transform_name == kRequireURL) {
    return RequireURLTransform::Create(transform_definition);
  }

  return nullptr;
}

std::optional<std::string> ApplyTransforms(
    const std::vector<std::unique_ptr<ValueTransform>>& transforms,
    std::string_view input) {
  std::optional<std::string> result(input);

  for (const auto& transform : transforms) {
    CHECK(transform);
    result = transform->Process(*result);
    if (!result) {
      return std::nullopt;
    }
  }

  return result;
}

}  // namespace web_discovery
