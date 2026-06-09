/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_PLAYLIST_EXCLUSIONS_H_
#define BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_PLAYLIST_EXCLUSIONS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/json/json_value_converter.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"

class GURL;

// Component payload `playlist_exclusions.json` schema (JSON object):
// {
//   "version": 1,
//   "rules": [
//     {
//       "registrable_domain": "youtube.com",
//       "deny_root_path": true,
//       "path_prefixes": ["/results", "/feed", "/@"]
//     }
//   ]
// }
// A URL is blocked (CanResolvePageSrcLater == false) when its
// registrable domain matches a rule AND (deny_root_path matches "" or "/" path
// OR path starts with any listed prefix). Note: Prefix "/" alone must not be
// used (use deny_root_path).

namespace playlist {

class PlaylistResolveRule {
 public:
  PlaylistResolveRule();
  ~PlaylistResolveRule();

  PlaylistResolveRule(const PlaylistResolveRule&);
  PlaylistResolveRule& operator=(const PlaylistResolveRule&);
  PlaylistResolveRule(PlaylistResolveRule&&) noexcept;
  PlaylistResolveRule& operator=(PlaylistResolveRule&&) noexcept;

  static void RegisterJSONConverter(
      base::JSONValueConverter<PlaylistResolveRule>* converter);

  std::string registrable_domain;
  std::vector<std::string> path_prefixes;
  bool deny_root_path = false;
};

class PlaylistExclusions {
 public:
  PlaylistExclusions(const PlaylistExclusions&) = delete;
  PlaylistExclusions& operator=(const PlaylistExclusions&) = delete;
  ~PlaylistExclusions();

  static PlaylistExclusions* GetInstance();

  // Reads and parses `exclusions_file_path` on a background thread pool task,
  // then updates the in-memory rules on the calling sequence. Until the first
  // successful parse, `CanResolvePageSrcLater` stays permissive. During
  // reloads, the last valid rules stay in effect; a failed parse leaves them
  // unchanged. `exclusions_file_path` must point to the component's
  // `playlist_exclusions.json` file. `on_complete` runs on the calling sequence
  // after the load attempt finishes.
  void LoadPlaylistExclusions(const base::FilePath& exclusions_file_path,
                              base::OnceClosure on_complete = {});

  // Returns true when `url` may be used for LivePlaylist-style reload /
  // re-resolution. Before the first successful load, returns true for all URLs
  // to avoid blocking playlist behavior.
  bool CanResolvePageSrcLater(const GURL& url) const;

 private:
  friend struct base::DefaultSingletonTraits<PlaylistExclusions>;
  friend class PlaylistExclusionsUnitTest;

  FRIEND_TEST_ALL_PREFIXES(PlaylistExclusionsUnitTest, NotReadyIsPermissive);
  FRIEND_TEST_ALL_PREFIXES(PlaylistExclusionsUnitTest,
                           FailedLoadsLeaveRulesEmpty);
  FRIEND_TEST_ALL_PREFIXES(PlaylistExclusionsUnitTest,
                           FailedReloadKeepsLastValidRules);
  FRIEND_TEST_ALL_PREFIXES(PlaylistExclusionsUnitTest, RulesBlockListedPaths);

  PlaylistExclusions();

  void OnPlaylistExclusionsLoaded(
      base::OnceClosure on_complete,
      std::optional<std::vector<PlaylistResolveRule>> rules);

  std::vector<PlaylistResolveRule> rules_;
  base::WeakPtrFactory<PlaylistExclusions> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_PLAYLIST_EXCLUSIONS_H_
