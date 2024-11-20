// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/core/youtube_rule.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/types/expected.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace {

// youtube.json keys
constexpr char kVersion[] = "version";
constexpr char kFeatureScript[] = "feature_script";

constexpr char kYouTubeUrl[] = "https://youtube.com";

// bool GetURLPatternSetFromValue(const base::Value* value,
//                                extensions::URLPatternSet* result) {
//   if (!value->is_list()) {
//     return false;
//   }
//   std::string error;
//   bool valid = result->Populate(value->GetList(), URLPattern::SCHEME_HTTPS,
//                                 false, &error);
//   if (!valid) {
//     DVLOG(1) << error;
//   }
//   return valid;
// }

bool GetFilePathFromValue(const base::Value* value, base::FilePath* result) {
  if (!value->is_string()) {
    return false;
  }
  auto val = value->GetString();
  *result = base::FilePath::FromASCII(val);
  return true;
}

}  // namespace

namespace youtube_script_injector {

YouTubeRule::YouTubeRule() = default;
YouTubeRule::~YouTubeRule() = default;
YouTubeRule::YouTubeRule(const YouTubeRule& other) {
  feature_script_path_ = other.feature_script_path_;
  version_ = other.version_;
}

// static
void YouTubeRule::RegisterJSONConverter(
    base::JSONValueConverter<YouTubeRule>* converter) {
  converter->RegisterCustomValueField<base::FilePath>(
      kFeatureScript, &YouTubeRule::feature_script_path_, GetFilePathFromValue);
  converter->RegisterIntField(kVersion, &YouTubeRule::version_);
}

// static
std::optional<YouTubeRule> YouTubeRule::ParseRules(
    const std::string& contents) {
  if (contents.empty()) {
    return std::nullopt;
  }
  std::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    VLOG(1) << "YouTubeRule::ParseRules: invalid JSON";
    return std::nullopt;
  }

  YouTubeRule rule = YouTubeRule();
  base::JSONValueConverter<YouTubeRule> converter;
  if (!converter.Convert(*root, &rule)) {
    VLOG(1) << "YouTubeRule::ParseRules: invalid rule";
    return std::nullopt;
  }
  return rule;
}

bool YouTubeRule::IsYouTubeDomain(const GURL& url) const {
  if (net::registry_controlled_domains::SameDomainOrHost(
          url, GURL(kYouTubeUrl),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return true;
  }

  return false;
}

}  // namespace youtube_script_injector
