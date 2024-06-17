/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"

#include <memory>
#include <optional>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

namespace brave {

namespace {
std::optional<extensions::URLPatternSet> CreateURLPatternSetFromList(
    const base::Value::List* value) {
  if (!value) {
    return std::nullopt;
  }
  extensions::URLPatternSet result;
  std::string error;
  bool valid = result.Populate(
      *value, URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, false,
      &error);
  if (!valid) {
    VLOG(1) << "Unable to create url pattern:" << error;
    return std::nullopt;
  }
  return result;
}

std::optional<base::flat_set<std::string>> CreateParamsList(
    const base::Value::List* value) {
  if (!value) {
    return std::nullopt;
  }
  base::flat_set<std::string> result;
  for (const auto& param : *value) {
    DCHECK(param.is_string());
    result.insert(param.GetString());
  }
  return result;
}

URLSanitizerService::Config ParseConfig(
    const URLSanitizerComponentInstaller::RawConfig& raw_config) {
  auto parsed_json =
      base::JSONReader::ReadAndReturnValueWithError(raw_config.matchers);
  if (!parsed_json.has_value()) {
    VLOG(1) << "Error parsing feature JSON [matchers]: "
            << parsed_json.error().message;
    return {};
  }
  const base::Value::List* list = parsed_json->GetIfList();
  if (!list) {
    return {};
  }

  URLSanitizerService::Config config;
  config.matchers.reserve(list->size());
  for (const auto& it : *list) {
    const base::Value::Dict* items = it.GetIfDict();
    if (!items) {
      continue;
    }
    auto include_matcher =
        CreateURLPatternSetFromList(items->FindList("include"));
    if (!include_matcher) {
      continue;
    }
    auto params = CreateParamsList(items->FindList("params"));
    if (!params) {
      continue;
    }

    auto exclude_matcher =
        CreateURLPatternSetFromList(items->FindList("exclude"))
            .value_or(extensions::URLPatternSet());

    config.matchers.emplace_back(std::move(*include_matcher),
                                 std::move(exclude_matcher),
                                 std::move(*params));
  }

  parsed_json =
      base::JSONReader::ReadAndReturnValueWithError(raw_config.permissions);
  if (!parsed_json.has_value()) {
    VLOG(1) << "Error parsing feature JSON [permission]: "
            << parsed_json.error().message;
    return config;
  }

  const auto* permissions = parsed_json->GetIfDict();
  if (!permissions) {
    return config;
  }

  config.permissions.js_api =
      CreateURLPatternSetFromList(parsed_json->GetDict().FindList("js_api"))
          .value_or(extensions::URLPatternSet());

  return config;
}

}  // namespace

URLSanitizerService::MatchItem::MatchItem() = default;
URLSanitizerService::MatchItem::MatchItem(MatchItem&&) = default;
URLSanitizerService::MatchItem::MatchItem(extensions::URLPatternSet in,
                                          extensions::URLPatternSet ex,
                                          base::flat_set<std::string> prm)
    : include(std::move(in)), exclude(std::move(ex)), params(std::move(prm)) {}
URLSanitizerService::MatchItem::~MatchItem() = default;

URLSanitizerService::Permissions::Permissions() = default;
URLSanitizerService::Permissions::Permissions(Permissions&&) = default;
URLSanitizerService::Permissions::~Permissions() = default;
URLSanitizerService::Permissions& URLSanitizerService::Permissions::operator=(
    Permissions&&) = default;

URLSanitizerService::Config::Config() = default;
URLSanitizerService::Config::Config(Config&&) = default;
URLSanitizerService::Config::~Config() = default;
URLSanitizerService::Config& URLSanitizerService::Config::operator=(Config&&) =
    default;

URLSanitizerService::URLSanitizerService() = default;

URLSanitizerService::~URLSanitizerService() = default;

#if BUILDFLAG(IS_ANDROID)
mojo::PendingRemote<url_sanitizer::mojom::UrlSanitizerService>
URLSanitizerService::MakeRemote() {
  mojo::PendingRemote<url_sanitizer::mojom::UrlSanitizerService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}
#endif  // # BUILDFLAG(IS_ANDROID)

void URLSanitizerService::SanitizeURL(const std::string& url,
                                      SanitizeURLCallback callback) {
  const auto& sanitized_url = SanitizeURL(GURL(url));
  std::move(callback).Run(sanitized_url.spec());
}

void URLSanitizerService::UpdateConfig(Config config) {
  config_ = std::move(config);
  if (initialization_callback_for_testing_) {
    std::move(initialization_callback_for_testing_).Run();
  }
}

GURL URLSanitizerService::SanitizeURL(const GURL& initial_url) {
  if (config_.matchers.empty() || !initial_url.SchemeIsHTTPOrHTTPS()) {
    return initial_url;
  }
  GURL url = initial_url;
  for (const auto& it : config_.matchers) {
    if (!it.include.MatchesURL(url) || it.exclude.MatchesURL(url)) {
      continue;
    }
    const auto sanitized_query = StripQueryParameter(url.query(), it.params);
    GURL::Replacements replacements;
    if (!sanitized_query.empty()) {
      replacements.SetQueryStr(sanitized_query);
    } else {
      replacements.ClearQuery();
    }
    url = url.ReplaceComponents(replacements);
  }
  return url;
}

bool URLSanitizerService::CheckJsPermission(const GURL& page_url) {
  for (const auto& p : config_.permissions.js_api) {
    if (p.MatchesURL(page_url)) {
      return true;
    }
  }
  return false;
}

void URLSanitizerService::OnConfigReady(
    const URLSanitizerComponentInstaller::RawConfig& config) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&ParseConfig, config),
      base::BindOnce(&URLSanitizerService::UpdateConfig,
                     weak_factory_.GetWeakPtr()));
}

// FIXME: merge with
// browser/net/brave_site_hacks_network_delegate_helper.cc::StripQueryParameter()
// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
std::string URLSanitizerService::StripQueryParameter(
    const std::string& query,
    const base::flat_set<std::string>& trackers) {
  // We are using custom query string parsing code here. See
  // https://github.com/brave/brave-core/pull/13726#discussion_r897712350
  // for more information on why this approach was selected.
  //
  // Split query string by ampersands, remove tracking parameters,
  // then join the remaining query parameters, untouched, back into
  // a single query string.
  const std::vector<std::string> input_kv_strings =
      SplitString(query, "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  std::vector<std::string> output_kv_strings;
  int disallowed_count = 0;
  for (const std::string& kv_string : input_kv_strings) {
    const std::vector<std::string> pieces = SplitString(
        kv_string, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const std::string& key = pieces.empty() ? std::string() : pieces[0];
    if (pieces.size() >= 2 && trackers.count(key) == 1) {
      ++disallowed_count;
    } else {
      output_kv_strings.push_back(kv_string);
    }
  }
  if (disallowed_count > 0) {
    return base::JoinString(output_kv_strings, "&");
  } else {
    return query;
  }
}

}  // namespace brave
