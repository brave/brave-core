// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { VoiceLanguageController } from './voice_language_controller-chromium.js'

// Bypass voice language installation requests on the ReadAnything WebUI.
Object.assign(VoiceLanguageController.prototype, {
  requestInstall_() {
    return false
  },
  requestInfo_() {},
})

export * from './voice_language_controller-chromium.js'
