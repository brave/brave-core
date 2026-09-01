// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_REGISTRY_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_REGISTRY_H_

#include <string_view>
#include <vector>

#include "base/containers/span.h"

namespace ai_chat::browser_settings {

// An allowlist of the preference paths the assistant is permitted to read.
//
// There is deliberately no description, label or category for each entry: the
// preference path is itself the description. Paths like
// "browser.clear_data.cookies_on_exit" or "brave.tabs.vertical_tabs_enabled"
// are self-describing enough for both the assistant and the search below, and
// keeping the registry to bare paths means adding a setting is a one-line
// change instead of a prose-writing exercise.
//
// The list was seeded from the settings_private allowlists that back
// brave://settings (chrome/browser/extensions/api/settings_private/
// prefs_util.cc and brave/browser/extensions/api/settings_private/
// brave_prefs_util.cc), filtered to scalar-valued preferences that exist on
// desktop Brave. It cannot be read from those files at runtime: prefs_util.cc
// lives inside `if (enable_extensions)` in the //chrome/browser/extensions
// monolith, which is not depend-able from here.
//
// Rules for adding entries:
//  - Only scalar preferences (boolean, number, string). Dictionaries and lists
//    hold structured user data and are rejected at read time anyway.
//  - Never anything that can hold free-form user content: filesystem paths,
//    user-entered hosts or URLs, custom filter lists, credentials or tokens.
//  - Never a preference that nothing reads. A vestigial preference reports a
//    stale value, which is worse than reporting nothing.

// Sorted, unique preference paths the assistant may read.
base::span<const std::string_view> GetAllowedPrefs();

bool IsAllowedPref(std::string_view pref_name);

struct SearchMatch {
  std::string_view pref_name;
  double score;
};

// Token-based fuzzy search over the allowlisted preference paths. Matches are
// ordered by descending score, ties broken by registry order, and truncated to
// `max_results`. Returns an empty vector when nothing scores above the
// relevance threshold.
std::vector<SearchMatch> SearchPrefs(std::string_view query,
                                     size_t max_results);

}  // namespace ai_chat::browser_settings

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_BROWSER_SETTINGS_REGISTRY_H_
