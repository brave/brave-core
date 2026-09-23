// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_COMPATIBILITY_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_COMPATIBILITY_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class ScriptMessage;
class WebState;
}  // namespace web

namespace playlist {

class PlaylistCompatibilityJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static PlaylistCompatibilityJavaScriptFeature* GetInstance();

  PlaylistCompatibilityJavaScriptFeature(
      const PlaylistCompatibilityJavaScriptFeature&) = delete;
  PlaylistCompatibilityJavaScriptFeature& operator=(
      const PlaylistCompatibilityJavaScriptFeature&) = delete;

  // JavaScriptFeature:
  bool GetFeatureRepliesToPrompts() const override;
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<PlaylistCompatibilityJavaScriptFeature>;

  web::MessageHandlerToken token_;

  PlaylistCompatibilityJavaScriptFeature();
  ~PlaylistCompatibilityJavaScriptFeature() override;
};

}  // namespace playlist

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_COMPATIBILITY_JAVASCRIPT_FEATURE_H_
