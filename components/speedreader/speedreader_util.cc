/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_util.h"

#include "base/strings/string_util.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace speedreader {

namespace {

// Regex pattern for paths like /blog/, /article/, /post/, hinting the page
// is a blog entry, magazine entry, or news article.
constexpr char kReadablePathSingleComponentHints[] =
    "(?i)/(blogs?|news|story|entry|articles?|posts?|amp)(/|$)";
// Regex pattern for matching URL paths of the form /YYYY/MM/DD/, which is
// extremely common for news websites.
constexpr char kReadablePathMultiComponentHints[] = "/\\d\\d\\d\\d/\\d\\d/";

constexpr char kReadableBlogSubdomain[] = "blog.";

}  // namespace

// private constructor
URLReadableHintExtractor::URLReadableHintExtractor()
    : path_single_component_hints_(kReadablePathSingleComponentHints),
      path_multi_component_hints_(kReadablePathMultiComponentHints) {
  DCHECK(path_single_component_hints_.ok());
  DCHECK(path_multi_component_hints_.ok());
}

bool URLReadableHintExtractor::HasHints(const GURL& url) {
  if (base::StartsWith(url.host_piece(), kReadableBlogSubdomain))
    return true;

  // Look for single components such as /blog/, /news/, /article/ and for
  // multi-path components like /YYYY/MM/DD
  if (re2::RE2::PartialMatch(url.path(), path_single_component_hints_) ||
      re2::RE2::PartialMatch(url.path(), path_multi_component_hints_)) {
    return true;
  }

  return false;
}

void SetSiteSpeedreadable(HostContentSettingsMap* map,
                          const GURL& url,
                          bool enable) {
  DCHECK(!url.is_empty());  // Not supported. Disable Speedreader in settings.

  // Rule covers all protocols and pages.
  auto pattern = ContentSettingsPattern::FromString("*://" + url.host() + "/*");
  if (!pattern.IsValid())
    return;

  ContentSetting setting =
      enable ? CONTENT_SETTING_BLOCK : CONTENT_SETTING_ALLOW;
  map->SetContentSettingCustomScope(pattern, ContentSettingsPattern::Wildcard(),
                                    ContentSettingsType::BRAVE_SPEEDREADER,
                                    setting);
}

bool IsEnabledForURL(HostContentSettingsMap* map, const GURL& url) {
  ContentSetting setting = map->GetContentSetting(
      url, GURL(), ContentSettingsType::BRAVE_SPEEDREADER);
  // Since Brave specific content settings are registered with
  // CONTENT_SETTINCG_BLOCK, the logic is reversed here. Sites that are
  // "allowed" are actually blacklisted by the user.
  // https://github.com/brave/brave-core/blob/1cb5818aa0b70666c6aeea5ea9c06cc4e712171a/chromium_src/components/content_settings/core/browser/content_settings_registry.cc#L37
  const bool enabled = setting == CONTENT_SETTING_BLOCK;
  return enabled;
}

}  // namespace speedreader
