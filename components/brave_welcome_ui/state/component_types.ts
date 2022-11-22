// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WebAnimationPlayer from '../api/web_animation_player'

export enum ViewType {
  DefaultBrowser, // the welcome ui
  ImportSelectBrowser,
  ImportSelectProfile,
  ImportInProgress,
  ImportSucceeded,
  ImportFailed,
  HelpImprove,
}

export enum BrowserType {
  Chrome_Canary = 'Chrome Canary',
  Chrome = 'Chrome',
  Chromium = 'Chromium',
  Safari = 'Safari',
  Mozilla_Firefox = 'Mozilla Firefox',
  Microsoft_Edge = 'Microsoft Edge',
  Vivaldi = 'Vivaldi',
  Opera = 'Opera'
}

export interface Scenes {
  s1: WebAnimationPlayer
  s2: WebAnimationPlayer
}
