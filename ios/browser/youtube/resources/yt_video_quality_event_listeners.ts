// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {functionAsListener} from '//ios/web/public/js_messaging/resources/utils.js';
import {requestVideoQualityPreference} from
    '//brave/ios/browser/youtube/resources/yt_video_quality_utils.js';

const allowedOrigins = [
  'https://youtube.com',
  'https://www.youtube.com',
  'https://m.youtube.com',
];

if (allowedOrigins.includes(window.location.origin)) {
  const listener = functionAsListener(requestVideoQualityPreference);

  // Apply quality when the video element finishes loading data (covers the
  // initial load as well as mid-session source changes).
  document.addEventListener('loadeddata', listener, {capture: true});

  // YouTube uses History API (pushState) for in-app navigation. The page
  // dispatches 'yt-navigate-finish' on document when each navigation
  // settles.
  document.addEventListener('yt-navigate-finish', listener);

  // Attempt to apply the highest quality immediately
  requestVideoQualityPreference();
}
