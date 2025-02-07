// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_JSON_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_JSON_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "extensions/common/url_pattern_set.h"

namespace youtube_script_injector {

// Format of the youtube.json file:
// {
//   "version": 1,
//   "playback_video_script": "playback-video.js",
//   "extra_controls_fullscreen_script": "extra-controls-fullscreen.js",
//   "extra_controls_pip_script": "extra-controls-pip.js"
// }
// Note that "playback_video_script_path_", and "extra_controls_fullscreen_script_path_",
// and "extra_controls_pip_script_path_" give a path relative to the
// component under scripts directory.
class YouTubeJson {
 public:
  YouTubeJson();
  ~YouTubeJson();
  YouTubeJson(
      const YouTubeJson& other);  // needed for std::optional<YouTubeJson>

  // Registers the mapping between JSON field names and the members in this
  // class.
  static void RegisterJSONConverter(
      base::JSONValueConverter<YouTubeJson>* converter);

  // Parse the youtube.json file contents into an optional YouTubeJson.
  static std::optional<YouTubeJson> ParseJson(const std::string& contents);

  // Getters.
  const base::FilePath& GetPlaybackVideoScript() const {
    return playback_video_script_path_;
  }
  const base::FilePath& GetFullscreenScript() const {
    return extra_controls_fullscreen_script_path_;
  }
  const base::FilePath& GetPipScript() const {
    return extra_controls_pip_script_path_;
  }
  int GetVersion() const { return version_; }

 private:
  // Thesse are paths (not contents) relative to the component under scripts/.
  base::FilePath playback_video_script_path_;
  base::FilePath extra_controls_fullscreen_script_path_;
  base::FilePath extra_controls_pip_script_path_;
  // Used for checking if the last inserted script is the latest version.
  int version_;
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_JSON_H_
