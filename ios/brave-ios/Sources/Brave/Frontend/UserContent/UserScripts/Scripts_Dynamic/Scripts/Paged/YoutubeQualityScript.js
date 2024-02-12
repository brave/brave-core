// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//<ytd-player id="ytd-player" context="WEB_PLAYER_CONTEXT_CONFIG_ID_KEVLAR_WATCH" class="style-scope ytd-watch-flexy" style="touch-action: pan-down;">
//  <div id="container" class="style-scope ytd-player"></div>
//  <div class="html5-video-player ytp-transparent ytp-fit-cover-video ytp-hide-info-bar" tabindex="-1" id="movie_player" aria-label="YouTube Video Player"></div>
//  <div class="html5-video-container" data-layer="0"></div>
//  <div class="ytp-gradient-top" data-layer="1"></div>
//</ytd-player>

window.__firefox__.includeOnce("YoutubeQuality", function($) {
  function debugPlayer(player) {
    console.log("List of Functions: ");
    for (var fn in player) {
      if (typeof player[fn] == 'function') {
        console.log(fn);
      }
    }
  }

  function findPlayer() {
    return document.getElementById('movie_player') || document.querySelector('.html5-video-player');
  }

  // Returns false if something failed in the process - we may retry after a small delay
  function updatePlayerQuality(player, requestedQuality) {
    if (!player || typeof player.getAvailableQualityLevels === 'undefined') {
      return false;
    }

    let qualities = player.getAvailableQualityLevels();
    if (qualities && qualities.length > 0 && requestedQuality.length > 0) {
      let quality = qualities.includes(requestedQuality) ? requestedQuality : qualities[0];

      if (player.setPlaybackQualityRange) {
        player.setPlaybackQualityRange(quality);
        return true;
      }

      if (player.setPlaybackQuality) {
        player.setPlaybackQuality(quality);
        return true;
      }

      return false;
    } else {
      // Sometimes the video qualities do not load fast enough.
      return false;
    }
  }

  var ytQualityTimerId = 0;
  var chosenQuality = "";

  Object.defineProperty(window.__firefox__, '$<set_youtube_quality>', {
    enumerable: false,
    configurable: false,
    writable: false,
    value: $(function(newVideoQuality) {
      // To not break the site completely, if it fails to upgrade few times we proceed with the default option.
      var attemptCount = 0;
      let maxAttempts = 3;

      chosenQuality = newVideoQuality;

      clearInterval(ytQualityTimerId);
      ytQualityTimerId = setInterval($(() => {
        let player = findPlayer();
        if (attemptCount++ > maxAttempts) {
          clearInterval(ytQualityTimerId);
          return;
        }

        if (updatePlayerQuality(player, chosenQuality)) {
          clearInterval(ytQualityTimerId);
        }
      }), 500);
    })
  });

  Object.defineProperty(window.__firefox__, '$<refresh_youtube_quality>', {
    enumerable: false,
    configurable: false,
    writable: false,
    value: $(function() {
      if (chosenQuality.length > 0) {
        window.__firefox__.$<set_youtube_quality>(chosenQuality);
      }
    })
  });

  $(function() {
    $.postNativeMessage('$<message_handler>', {
      "securityToken": SECURITY_TOKEN,
      "request": "get_default_quality"
    }).then($(function(quality) {
      window.__firefox__.$<set_youtube_quality>(quality);
    }));
  })();
});
