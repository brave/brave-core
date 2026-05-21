// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessageWithReply} from
    '//ios/web/public/js_messaging/resources/utils.js';

function findPlayer(): any {
  return document.getElementById('movie_player') ||
      document.querySelector('.html5-video-player');
}

export function applyHighestQuality(): void {
  const player = findPlayer();
  if (!player || typeof player.getAvailableQualityLevels === 'undefined' ||
      !player.setPlaybackQualityRange) {
    return;
  }
  const qualities: string[] = player.getAvailableQualityLevels();
  if (qualities && qualities.length > 0) {
    player.setPlaybackQualityRange(qualities[0], qualities[0]);
  }
}

export function resetQuality(): void {
  const player = findPlayer();
  if (!player || !player.setPlaybackQualityRange) {
    return;
  }
  player.setPlaybackQualityRange('auto', 'auto');
}

export function requestVideoQualityPreference() {
  sendWebKitMessageWithReply('YouTubeQualityMessageHandler', {})
    .then((shouldApplyHighestQuality: boolean) => {
      if (shouldApplyHighestQuality) {
        applyHighestQuality();
      }
    });
}
