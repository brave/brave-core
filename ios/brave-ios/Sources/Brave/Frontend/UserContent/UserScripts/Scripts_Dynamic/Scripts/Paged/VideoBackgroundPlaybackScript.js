// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Used while kUseProfileWebViewConfiguration is off. The Chromium feature
// video_background_playback.ts is used when that flag is on.

window.__firefox__.includeOnce("VideoBackgroundPlayback", function($) {
  // The audible video that was playing while the scene resigned active, before
  // WebKit pauses video for backgrounding. One per document so a second play()
  // does not restart its buffer.
  var playingVideo = null;

  function isAudibleAndPlaying(video) {
    return !video.paused && !video.ended && !video.muted && video.volume > 0;
  }

  function findPlayingVideo(root) {
    var videos = root.querySelectorAll("video");
    for (var i = 0; i < videos.length; i++) {
      if (isAudibleAndPlaying(videos[i])) {
        return videos[i];
      }
    }
    var elements = root.querySelectorAll("*");
    for (var j = 0; j < elements.length; j++) {
      if (!elements[j].shadowRoot) {
        continue;
      }
      var found = findPlayingVideo(elements[j].shadowRoot);
      if (found) {
        return found;
      }
    }
    return null;
  }

  Object.defineProperty(window.__firefox__, "$<mark_playing_videos>", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: $(function() {
      playingVideo = findPlayingVideo(document);
    })
  });

  Object.defineProperty(window.__firefox__, "$<resume_playing_videos>", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: $(function() {
      var video = playingVideo;
      if (!video) {
        return;
      }
      var resume = $(function() {
        if (playingVideo !== video || video.ended || !video.paused) {
          return;
        }
        playingVideo = null;
        video.play().catch(function() {});
      });
      // WebKit applies its background pause after the scene enters the
      // background, so the video may still be playing when this runs. Allow
      // for a small delay before resuming.
      setTimeout(resume, 250);
    })
  });
});
