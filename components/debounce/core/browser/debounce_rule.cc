// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/core/browser/debounce_rule.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/types/expected.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace {
// debounce.json keys
const char kInclude[] = "include";
const char kExclude[] = "exclude";
const char kAction[] = "action";
const char kPrependScheme[] = "prepend_scheme";
const char kParam[] = "param";
const char kPref[] = "pref";

// Max memory per regex: 4kb. This is just an upper bound
const int64_t kMaxMemoryPerRegexPattern = 2 << 11;

// Max length of regex pattern
// RE2 is O(n) for input string of length n
// (https://github.com/google/re2/wiki/WhyRE2)
// Max size of a URL is capped anyway.
// Also cap the length of regex pattern to be extra safe
const int64_t kMaxLengthRegexPattern = 200;

// Removes trailing dot from |host_piece| if any.
// Copied from extensions/common/url_pattern.cc
std::string_view CanonicalizeHostForMatching(std::string_view host_piece) {
  if (base::EndsWith(host_piece, ".")) {
    host_piece.remove_suffix(1);
  }
  return host_piece;
}

// Extract the host from |url| using a simple parsing algorithm
// WARNING: this is a special-purpose function whose output should not be used.
std::string NaivelyExtractHostnameFromUrl(const std::string& url) {
  CHECK(GURL(url).SchemeIsHTTPOrHTTPS());

  const std::string kHttp =
      base::StrCat({url::kHttpScheme, url::kStandardSchemeSeparator});
  const std::string kHttps =
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator});
  std::string mutable_url = url;

  if (base::StartsWith(mutable_url, kHttps,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    mutable_url = mutable_url.substr(kHttps.length());
  } else if (base::StartsWith(mutable_url, kHttp,
                              base::CompareCase::INSENSITIVE_ASCII)) {
    mutable_url = mutable_url.substr(kHttp.length());
  }

  // Known limitation: this will not work properly with origins which consist
  // of IPv6 hostnames.
  const std::vector<std::string> parts = base::SplitString(
      mutable_url, ":/", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (parts.size() > 0) {
    return parts[0];
  }

  return std::string();
}

}  // namespace

namespace debounce {

DebounceRule::DebounceRule()
    : action_(kDebounceNoAction), prepend_scheme_(kDebounceNoSchemePrepend) {}

DebounceRule::~DebounceRule() = default;

// static
bool DebounceRule::ParseDebounceAction(std::string_view value,
                                       DebounceAction* field) {
  if (value == "redirect") {
    *field = kDebounceRedirectToParam;
  } else if (value == "base64,redirect") {
    *field = kDebounceBase64DecodeAndRedirectToParam;
  } else if (value == "regex-path") {
    *field = kDebounceRegexPath;
  } else {
    VLOG(1) << "Found unknown debouncing action: " << value;
    return false;
  }
  return true;
}

// static
bool DebounceRule::ParsePrependScheme(std::string_view value,
                                      DebouncePrependScheme* field) {
  if (value == "http") {
    *field = kDebounceSchemePrependHttp;
  } else if (value == "https") {
    *field = kDebounceSchemePrependHttps;
  } else {
    VLOG(1) << "Found unknown scheme: " << value;
    return false;
  }
  return true;
}

// static
bool DebounceRule::GetURLPatternSetFromValue(
    const base::Value* value,
    extensions::URLPatternSet* result) {
  if (!value->is_list()) {
    return false;
  }
  // Debouncing only affects HTTP or HTTPS URLs, regardless of how the rules are
  // written. (Also, don't write rules for other URL schemes, because they won't
  // work and you're just wasting everyone's time.)
  std::string error;
  bool valid = result->Populate(
      value->GetList(), URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      false, &error);
  if (!valid) {
    VLOG(1) << error;
  }
  return valid;
}

// static
void DebounceRule::RegisterJSONConverter(
    base::JSONValueConverter<DebounceRule>* converter) {
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kInclude, &DebounceRule::include_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kExclude, &DebounceRule::exclude_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomField<DebounceAction>(
      kAction, &DebounceRule::action_, &ParseDebounceAction);
  converter->RegisterCustomField<DebouncePrependScheme>(
      kPrependScheme, &DebounceRule::prepend_scheme_, &ParsePrependScheme);
  converter->RegisterStringField(kParam, &DebounceRule::param_);
  converter->RegisterStringField(kPref, &DebounceRule::pref_);
}

// static
// All eTLD+1 calculations for debouncing should flow through here so they
// are consistent in their private registries configuration.
const std::string DebounceRule::GetETLDForDebounce(const std::string& host) {
  std::string_view host_piece = CanonicalizeHostForMatching(host);
  return net::registry_controlled_domains::GetDomainAndRegistry(
      host_piece, net::registry_controlled_domains::PrivateRegistryFilter::
                      EXCLUDE_PRIVATE_REGISTRIES);
}

// static
bool DebounceRule::IsSameETLDForDebounce(const GURL& url1, const GURL& url2) {
  return net::registry_controlled_domains::SameDomainOrHost(
      url1, url2,
      net::registry_controlled_domains::PrivateRegistryFilter::
          EXCLUDE_PRIVATE_REGISTRIES);
}

// static
base::expected<std::pair<std::vector<std::unique_ptr<DebounceRule>>,
                         base::flat_set<std::string>>,
               std::string>
DebounceRule::ParseRules(const std::string& contents) {
  if (contents.empty()) {
    return base::unexpected("Could not obtain debounce configuration");
  }
  std::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    return base::unexpected("Failed to parse debounce configuration");
  }
  std::vector<std::string> hosts;
  std::vector<std::unique_ptr<DebounceRule>> rules;
  base::JSONValueConverter<DebounceRule> converter;
  for (base::Value& it : root->GetList()) {
    std::unique_ptr<DebounceRule> rule = std::make_unique<DebounceRule>();
    if (!converter.Convert(it, rule.get())) {
      continue;
    }
    for (const URLPattern& pattern : rule->include_pattern_set()) {
      if (!pattern.host().empty()) {
        const std::string etldp1 =
            DebounceRule::GetETLDForDebounce(pattern.host());
        if (!etldp1.empty()) {
          hosts.push_back(std::move(etldp1));
        }
      }
    }
    rules.push_back(std::move(rule));
  }
  return std::pair<std::vector<std::unique_ptr<DebounceRule>>,
                   base::flat_set<std::string>>(std::move(rules), hosts);
}

bool DebounceRule::CheckPrefForRule(const PrefService* prefs) const {
  // Check pref specified in rules, if any
  if (!pref_.empty()) {
    auto* pref = prefs->FindPreference(pref_);
    if (!pref) {
      VLOG(1) << "Pref specified in debounce.json not valid: " << pref_;
      return false;
    }
    if (!pref->GetValue()->GetBool()) {
      VLOG(1) << "Pref " << pref->name()
              << " specified in debounce.json is false";
      return false;
    }
  }
  return true;
}

bool DebounceRule::ValidateAndParsePatternRegex(
    const std::string& pattern,
    const std::string& path,
    std::string* parsed_value) const {
  if (pattern.length() > kMaxLengthRegexPattern) {
    VLOG(1) << "Debounce regex pattern exceeds max length: "
            << kMaxLengthRegexPattern;
    return false;
  }
  re2::RE2::Options options;
  options.set_max_mem(kMaxMemoryPerRegexPattern);
  const re2::RE2 pattern_regex(pattern, options);

  if (!pattern_regex.ok()) {
    VLOG(1) << "Debounce rule has param: " << pattern
            << " which is an invalid regex pattern";
    return false;
  }
  if (pattern_regex.NumberOfCapturingGroups() < 1) {
    VLOG(1) << "Debounce rule has param: " << pattern
            << " which captures < 1 groups";
    return false;
  }

  // Get matching capture groups by applying regex to the path
  size_t number_of_capturing_groups =
      pattern_regex.NumberOfCapturingGroups() + 1;
  std::vector<std::string_view> match_results(number_of_capturing_groups);

  if (!pattern_regex.Match(path, 0, path.size(), RE2::UNANCHORED,
                           match_results.data(), match_results.size())) {
    VLOG(1) << "Debounce rule with param: " << param_
            << " was unable to capture string";
    return false;
  }

  // This will always be at least 2: the first one is the full match
  DCHECK_GT(match_results.size(), 1u);

  // Build parsed_value string by appending matches, ignoring the first match
  // which will be the whole match
  std::for_each(std::begin(match_results) + 1, std::end(match_results),
                [parsed_value](std::string_view matched_string) {
                  if (!matched_string.empty()) {
                    parsed_value->append(matched_string);
                  }
                });

  return true;
}

bool DebounceRule::Apply(const GURL& original_url,
                         GURL* final_url,
                         const PrefService* prefs) const {
  // Unknown actions always return false, to allow for future updates to the
  // rules file which may be pushed to users before a new version of the code
  // that parses it.
  if (action_ != kDebounceRedirectToParam &&
      action_ != kDebounceBase64DecodeAndRedirectToParam &&
      action_ != kDebounceRegexPath) {
    return false;
  }
  // If URL matches an explicitly excluded pattern, this rule does not apply.
  if (exclude_pattern_set_.MatchesURL(original_url)) {
    return false;
  }
  // If URL does not match an explicitly included pattern, this rule does not
  // apply.
  if (!include_pattern_set_.MatchesURL(original_url)) {
    return false;
  }

  if (!DebounceRule::CheckPrefForRule(prefs)) {
    return false;
  }

  std::string unescaped_value;

  if (action_ == kDebounceRegexPath) {
    // Important: Apply param regex to ONLY the path of original URL.
    auto path = original_url.path();

    if (!ValidateAndParsePatternRegex(param_, path, &unescaped_value)) {
      VLOG(1) << "Debounce regex parsing failed";
      return false;
    }

    // Unescape the URL
    // This is identical to QueryIterator::GetUnescapedValue() in url_util.cc,
    // which is what is used for query parameters when we call
    // net::GetValueForKeyInQuery
    if (!unescaped_value.empty()) {
      unescaped_value = base::UnescapeURLComponent(
          unescaped_value,
          base::UnescapeRule::SPACES | base::UnescapeRule::PATH_SEPARATORS |
              base::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
              base::UnescapeRule::REPLACE_PLUS_WITH_SPACE);
    }

    // unescaped_value now has a string; we will check if the captured value is
    // a valid URL down below
  } else {
    if (!net::GetValueForKeyInQuery(original_url, param_, &unescaped_value)) {
      return false;
    }
    if ((action_ == kDebounceBase64DecodeAndRedirectToParam) &&
        (!base::Base64UrlDecode(unescaped_value,
                                base::Base64UrlDecodePolicy::IGNORE_PADDING,
                                &unescaped_value))) {
      return false;
    }
  }

  std::string new_url_spec = unescaped_value;
  GURL new_url(new_url_spec);
  // Important: If there is a prepend_scheme in the rule BUT the URL is already
  // valid i.e. has a scheme, we treat this as an erroneous rule and do not
  // apply it.
  if (prepend_scheme_ && new_url.is_valid()) {
    VLOG(1) << "Debounce rule with param: " << param_ << " and prepend scheme "
            << prepend_scheme_
            << " got a valid URL, treating as erroneous rule";
    return false;
  }
  // If there is a prepend_scheme specified AND the URL is not valid, prepend
  // the specified scheme and try again
  if (prepend_scheme_ && !new_url.is_valid()) {
    std::string scheme = (prepend_scheme_ == kDebounceSchemePrependHttp)
                             ? url::kHttpScheme
                             : url::kHttpsScheme;
    new_url_spec =
        base::StringPrintf("%s://%s", scheme.c_str(), unescaped_value.c_str());
    new_url = GURL(new_url_spec);
    if (new_url.is_valid()) {
      DCHECK(new_url.scheme() == scheme);
    }
  }

  // Failsafe: ensure we got a valid URL out of the param.
  if (!new_url.is_valid() || !new_url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  // Failsafe: never redirect to the same site.
  if (IsSameETLDForDebounce(original_url, new_url)) {
    return false;
  }

  // If the hostname of the new url as extracted via our simple parser doesn't
  // match the host as parsed via GURL, this rule does not apply
  if (NaivelyExtractHostnameFromUrl(new_url_spec) != new_url.host()) {
    return false;
  }

  *final_url = new_url;
  return true;
}

}  // namespace debounce
