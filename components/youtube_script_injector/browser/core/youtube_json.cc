// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/types/expected.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace {

// youtube.json keys
constexpr char kVersion[] = "version";
constexpr char kFullscreenScript[] = "extra_controls_fullscreen_script";
constexpr char kPipScript[] = "extra_controls_pip_script";
constexpr char kPlaybackVideoScript[] = "playback_video_script";

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

YouTubeJson::YouTubeJson() = default;
YouTubeJson::~YouTubeJson() = default;
YouTubeJson::YouTubeJson(const YouTubeJson& other) {
  playback_video_script_path_ = other.playback_video_script_path_;
  extra_controls_fullscreen_script_path_ =
      other.extra_controls_fullscreen_script_path_;
  extra_controls_pip_script_path_ = other.extra_controls_pip_script_path_;
  version_ = other.version_;
}

const base::FilePath& YouTubeJson::GetScript(ScriptType type) const {
  switch (type) {
    case ScriptType::PLAYBACK_VIDEO:
      return playback_video_script_path_;
    case ScriptType::FULLSCREEN:
      return extra_controls_fullscreen_script_path_;
    case ScriptType::PIP:
      return extra_controls_pip_script_path_;
  }
  NOTREACHED();
}

// static
void YouTubeJson::RegisterJSONConverter(
    base::JSONValueConverter<YouTubeJson>* converter) {
  converter->RegisterCustomValueField<base::FilePath>(
      kPlaybackVideoScript, &YouTubeJson::playback_video_script_path_,
      GetFilePathFromValue);
  converter->RegisterCustomValueField<base::FilePath>(
      kFullscreenScript, &YouTubeJson::extra_controls_fullscreen_script_path_,
      GetFilePathFromValue);
  converter->RegisterCustomValueField<base::FilePath>(
      kPipScript, &YouTubeJson::extra_controls_pip_script_path_,
      GetFilePathFromValue);
  converter->RegisterIntField(kVersion, &YouTubeJson::version_);
}

// static
std::optional<YouTubeJson> YouTubeJson::ParseJson(const std::string& contents) {
  if (contents.empty()) {
    return std::nullopt;
  }
  std::optional<base::Value> root = base::JSONReader::Read(contents);
  if (!root) {
    VLOG(1) << "YouTubeJson::ParseJson: invalid JSON";
    return std::nullopt;
  }

  YouTubeJson rule = YouTubeJson();
  base::JSONValueConverter<YouTubeJson> converter;
  if (!converter.Convert(*root, &rule)) {
    VLOG(1) << "YouTubeJson::YouTubeJson: invalid rule";
    return std::nullopt;
  }
  return rule;
}

}  // namespace youtube_script_injector
