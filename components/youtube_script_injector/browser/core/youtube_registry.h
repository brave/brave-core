// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_REGISTRY_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_REGISTRY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"

class GURL;

namespace youtube_script_injector {
// Needed for testing private methods in YouTubeScriptInjectorTabFeatureBrowserTest.
FORWARD_DECLARE_TEST(YouTubeScriptInjectorTabFeatureBrowserTest, TestLoadJsonPlayback);
FORWARD_DECLARE_TEST(YouTubeScriptInjectorTabFeatureBrowserTest, TestLoadJsonExtraControls);
FORWARD_DECLARE_TEST(YouTubeScriptInjectorTabFeatureBrowserTest, TestLoadJson);

// This class loads and stores the rules from the youtube.json file.
// It is also used for matching based on the URL.
// - See `youtube_json.h` for an example of the JSON this class loads
// - See `youtube_component_installer.cc` for example of the component layout.
//        This the location where the scripts are loaded from.
class COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE) YouTubeRegistry {
 public:
  YouTubeRegistry(const YouTubeRegistry&) = delete;
  YouTubeRegistry& operator=(const YouTubeRegistry&) = delete;
  ~YouTubeRegistry();
  static YouTubeRegistry* GetInstance();  // singleton
  // Returns the YouTube script content from a given path.
  void LoadScriptFromPath(const GURL& url,
                          const base::FilePath& script_path,
                          base::OnceCallback<void(std::string)> cb) const;
  // Given a path to youtube.json, loads the scripts from the file into memory.
  void LoadJson(const base::FilePath& path);
  const std::optional<YouTubeJson>& GetJson() const { return json_; }
  static bool IsYouTubeDomain(const GURL& url);

 private:
  YouTubeRegistry();

  // These methods are also called by YouTubeTabFeatureBrowserTest.
  // Given contents of youtube.json, loads the Json scripts from files into
  // memory. Called by |LoadJson| after the file is read.
  void OnLoadJson(const std::string& data);
  // Sets the component path used to resolve the paths to the scripts.
  void SetComponentPath(const base::FilePath& path);

  base::FilePath component_path_;
  std::optional<YouTubeJson> json_;

  base::WeakPtrFactory<YouTubeRegistry> weak_factory_{this};

  // Needed for testing private methods in YouTubeScriptInjectorTabFeatureBrowserTest.
  FRIEND_TEST_ALL_PREFIXES(YouTubeScriptInjectorTabFeatureBrowserTest, TestLoadJsonPlayback);
  FRIEND_TEST_ALL_PREFIXES(YouTubeScriptInjectorTabFeatureBrowserTest, TestLoadJsonExtraControls);
  FRIEND_TEST_ALL_PREFIXES(YouTubeScriptInjectorTabFeatureBrowserTest, TestLoadJson);
  friend class YouTubeScriptInjectorTabFeatureBrowserTest;

  friend struct base::DefaultSingletonTraits<YouTubeRegistry>;
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_REGISTRY_H_
