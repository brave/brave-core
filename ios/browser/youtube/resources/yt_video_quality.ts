// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from
    '//ios/web/public/js_messaging/resources/gcrweb.js';
import {resetQuality} from
    '//brave/ios/browser/youtube/resources/yt_video_quality_utils.js';

const youtubeQualityApi = new CrWebApi('youtubeQuality');
youtubeQualityApi.addFunction('resetQuality', resetQuality);
gCrWeb.registerApi(youtubeQualityApi);
