// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class ScriptMessage;
class WebState;
}  // namespace web

namespace playlist {

// A JavaScriptFeature that detects playable media (<video>/<audio>) on a page
// and reports each detected item to the browser. It injects two scripts into
// the page content world:
//   * `playlist` — exposes the media query API and installs hooks that catch
//     media added to the page dynamically. Injected once per window.
//   * `playlist_event_listeners` — scans the document for media when it loads.
//     Re-injected on every document recreation.
// Ported from the original //brave-ios `PlaylistScript.js`.
class PlaylistJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static PlaylistJavaScriptFeature* GetInstance();

  PlaylistJavaScriptFeature(const PlaylistJavaScriptFeature&) = delete;
  PlaylistJavaScriptFeature& operator=(const PlaylistJavaScriptFeature&) =
      delete;

  // Queries the current playback time, in seconds, of the media element in the
  // main frame of `web_state` that was tagged with `tag`. `callback` receives
  // 0 when no matching element exists or the call fails.
  void GetCurrentTimeForVideoWithTag(web::WebState* web_state,
                                     const std::string& tag,
                                     base::OnceCallback<void(double)> callback);

  // Detects media at the (`x`, `y`) point that the user long pressed, in the
  // main frame's coordinate space of `web_state`. Any detected media is
  // reported asynchronously through the registered message handler (see
  // ScriptMessageReceived), flagged as not automatically detected.
  void LongPressedAtLocation(web::WebState* web_state, double x, double y);

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override;

 private:
  friend class base::NoDestructor<PlaylistJavaScriptFeature>;

  PlaylistJavaScriptFeature();
  ~PlaylistJavaScriptFeature() override;
};

}  // namespace playlist

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_JAVASCRIPT_FEATURE_H_
