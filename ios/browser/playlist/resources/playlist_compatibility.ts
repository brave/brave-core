// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendTokenizedWebKitMessageSynchronously } from '//brave/ios/web/js_messaging/resources/utils.js'

declare global {
  interface Window {
    ManagedMediaSource: any
    MediaSource: any
  }
}

const isRunningCompatibilityMode = sendTokenizedWebKitMessageSynchronously(
  'PlaylistCompatibilityMessageHandler',
  {},
) as boolean

if (isRunningCompatibilityMode) {
  if (window.MediaSource || window.ManagedMediaSource) {
    delete window.MediaSource
    delete window.ManagedMediaSource
  }
}
