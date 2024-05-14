// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WebAnimationPlayer from '../api/web_animation_player'

export enum ViewType {
  DefaultBrowser, // the welcome ui
  ImportSelectBrowser,
  ImportSelectTheme,
  ImportSelectProfile,
  ImportInProgress,
  ImportSucceeded,
  ImportFailed,
  HelpImprove,
  HelpWDP,
}

export enum BrowserType {
  Chrome_Canary = 'Google Chrome Canary',
  Chrome_Beta = 'Google Chrome Beta',
  Chrome_Dev = 'Google Chrome Dev',
  Chrome = 'Google Chrome',
  Chromium = 'Chromium',
  Safari = 'Safari',
  Mozilla_Firefox = 'Firefox',
  Microsoft_Edge = 'Microsoft Edge',
  Vivaldi = 'Vivaldi',
  Opera = 'Opera',
  Yandex = 'Yandex',
  Whale = 'NAVER Whale',
  Microsoft_IE = 'Microsoft Internet Explorer'
}

export interface Scenes {
  s1: WebAnimationPlayer
  s2: WebAnimationPlayer
}
