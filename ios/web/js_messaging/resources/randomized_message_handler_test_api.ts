// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { messageHandlerName } from '//brave/ios/web/js_messaging/resources/utils.js'
import {
  CrWebApi,
  gCrWeb,
} from '//ios/web/public/js_messaging/resources/gcrweb.js'
import { sendWebKitMessage } from '//ios/web/public/js_messaging/resources/utils.js'

function send() {
  sendWebKitMessage(messageHandlerName, {})
}

const testApi = new CrWebApi('randomized_message_handler_tests')
testApi.addFunction('sendWebKitMessage', send)
gCrWeb.registerApi(testApi)
