// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.includeOnce('FullscreenHelper', function($) {
  function canEnterFullscreen(video) {
    return video && video.webkitSupportsFullscreen !== false &&
        (video.webkitEnterFullscreen !== undefined ||
         video.webkitSetPresentationMode !== undefined);
  }

  function findVideoInRoot(root) {
    var videos = root.querySelectorAll('video');
    var i;
    for (i = 0; i < videos.length; i++) {
      if (canEnterFullscreen(videos[i]) && !videos[i].paused) {
        return videos[i];
      }
    }
    for (i = 0; i < videos.length; i++) {
      if (canEnterFullscreen(videos[i])) {
        return videos[i];
      }
    }
    var elements = root.querySelectorAll('*');
    for (i = 0; i < elements.length; i++) {
      if (elements[i].shadowRoot) {
        var nested = findVideoInRoot(elements[i].shadowRoot);
        if (nested) {
          return nested;
        }
      }
    }
    var iframes = root.querySelectorAll('iframe');
    for (i = 0; i < iframes.length; i++) {
      try {
        if (iframes[i].contentDocument) {
          var frameVideo = findVideoInRoot(iframes[i].contentDocument);
          if (frameVideo) {
            return frameVideo;
          }
        }
      } catch (e) {
      }
    }
    return null;
  }

  function findVideo(start) {
    if (canEnterFullscreen(start) && start.localName === 'video') {
      return start;
    }

    var current = start;
    while (current) {
      var found = findVideoInRoot(current);
      if (found) {
        return found;
      }
      var root = current.getRootNode();
      if (root && root.host) {
        current = root.host;
        continue;
      }
      current = current.parentElement;
    }
    return findVideoInRoot(document);
  }

  function enterVideoFullscreen(video) {
    try {
      if (video.webkitSetPresentationMode) {
        video.webkitSetPresentationMode('fullscreen');
        return true;
      }
      if (video.webkitEnterFullscreen) {
        video.webkitEnterFullscreen();
        return true;
      }
    } catch (e) {
    }
    return false;
  }

  function requestVideoFullscreen(element) {
    var video = findVideo(element);
    return !!video && enterVideoFullscreen(video);
  }

  let isFullscreenSupportedNatively = document.fullscreenEnabled ||
      document.webkitFullscreenEnabled || document.mozFullScreenEnabled ||
      document.msFullscreenEnabled;

  let videosSupportFullscreen = HTMLVideoElement.prototype.webkitEnterFullscreen !== undefined

  if (!isFullscreenSupportedNatively && videosSupportFullscreen && !/mobile/i.test(navigator.userAgent)) {

    HTMLElement.prototype.requestFullscreen = $(function() {
      return requestVideoFullscreen(this);
    });

    HTMLElement.prototype.webkitRequestFullscreen = $(function() {
      requestVideoFullscreen(this);
    });

    Object.defineProperty(document, 'fullscreenEnabled', {
      get: function() {
        return true;
      }
    });

    Object.defineProperty(document.documentElement, 'fullscreenEnabled', {
      get: function() {
        return true;
      }
    });
  }
});
