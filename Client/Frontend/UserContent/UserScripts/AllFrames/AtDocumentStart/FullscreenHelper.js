// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

window.__firefox__.includeOnce("FullscreenHelper", function($) {
  let isFullscreenSupportedNatively = document.fullscreenEnabled ||
                                      document.webkitFullscreenEnabled ||
                                      document.mozFullScreenEnabled ||
                                      document.msFullscreenEnabled ? true : false;
  
  let videosSupportFullscreen = HTMLVideoElement.prototype.webkitEnterFullscreen !== undefined

  if (!isFullscreenSupportedNatively && videosSupportFullscreen && !/mobile/i.test(navigator.userAgent)) {
      
    HTMLElement.prototype.requestFullscreen = $(function() {
      if (this.webkitRequestFullscreen !== undefined) {
        this.webkitRequestFullscreen();
        return true;
      }
      
      if (this.webkitEnterFullscreen !== undefined) {
        this.webkitEnterFullscreen();
        return true;
      }
      
      var video = this.querySelector("video")
      if (video !== undefined) {
        video.webkitEnterFullscreen();
        return true;
      }
      return false;
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
