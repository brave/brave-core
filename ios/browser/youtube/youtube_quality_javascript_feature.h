// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_YOUTUBE_YOUTUBE_QUALITY_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_YOUTUBE_YOUTUBE_QUALITY_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

namespace youtube {

// A JavaScriptFeature that injects YouTube enhancement scripts into YouTube
// pages. It handles video quality control by replying to boolean quality
// queries from the page — whether to pin playback to the highest available
// level or use automatic quality selection.
class YouTubeQualityJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static YouTubeQualityJavaScriptFeature* GetInstance();

  // Reverts the playback quality on the main frame of |web_state| to YouTube's
  // automatic quality selection. Intended for use when a network condition
  // change (e.g. leaving Wi-Fi) makes the current quality preference no longer
  // applicable.
  void ResetQuality(web::WebState* web_state);

  // JavaScriptFeature:
  bool GetFeatureRepliesToMessages() const override;
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<YouTubeQualityJavaScriptFeature>;

  YouTubeQualityJavaScriptFeature();
  ~YouTubeQualityJavaScriptFeature() override;

  YouTubeQualityJavaScriptFeature(const YouTubeQualityJavaScriptFeature&) =
      delete;
  YouTubeQualityJavaScriptFeature& operator=(
      const YouTubeQualityJavaScriptFeature&) = delete;
};

}  // namespace youtube

#endif  // BRAVE_IOS_BROWSER_YOUTUBE_YOUTUBE_QUALITY_JAVASCRIPT_FEATURE_H_
