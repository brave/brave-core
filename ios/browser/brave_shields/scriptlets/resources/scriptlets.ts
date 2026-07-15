// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  sendTokenizedWebKitMessageSynchronously,
  messageHandlerName,
} from '//brave/ios/web/js_messaging/resources/utils.js'

const scriptlets: string[] = sendTokenizedWebKitMessageSynchronously(
  messageHandlerName,
  {},
)

for (const scriptlet of scriptlets) {
  new Function(scriptlet)()
}
