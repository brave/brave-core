/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/value_transform.h"

#include <string>
#include <utility>

#include "base/containers/flat_set.h"
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
constexpr std::string_view kFilterExact = "filterExact";
constexpr std::string_view kMaskU = "maskU";
constexpr std::string_view kSplit = "split";
constexpr std::string_view kTrim = "trim";
constexpr std::string_view kRelaxedMaskU = "relaxedMaskU";

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
  } else if (transform_name == kFilterExact) {
    return FilterExactTransform::Create(transform_definition);
  } else if (transform_name == kMaskU) {
    return MaskUTransform::Create(transform_definition, false);
  } else if (transform_name == kRelaxedMaskU) {
    return MaskUTransform::Create(transform_definition, true);
  } else if (transform_name == kTrim) {
    return TrimTransform::Create(transform_definition);
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
