/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_PLAYLIST_EXCLUSION_H_
#define BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_PLAYLIST_EXCLUSION_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

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

  void OnComponentReady(const base::FilePath& component_dir);

  // Returns true when `pageSrc` may be used for LivePlaylist-style reload /
  // re-resolution. When the component list is not loaded yet, returns true
  bool CanResolvePageSrcLater(const GURL& url) const;

  // Debugging / UI: human-readable "domain<TAB>condition" rows.
  std::vector<std::string> ListPlaylistExclusions() const;

  void ResetForTesting();

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistExclusionsUnitTest, RulesBlockListedPaths);
  FRIEND_TEST_ALL_PREFIXES(PlaylistExclusionsUnitTest, NotReadyIsPermissive);

  PlaylistExclusions();

  void OnPlaylistExclusionsLoaded(const std::string& contents);

  base::FilePath component_path_;
  std::vector<PlaylistResolveRule> rules_;
  bool is_ready_ = false;
  base::WeakPtrFactory<PlaylistExclusions> weak_factory_{this};

  friend struct base::DefaultSingletonTraits<PlaylistExclusions>;
  friend class PlaylistExclusionsUnitTest;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_PLAYLIST_EXCLUSION_H_
