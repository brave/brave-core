// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from '//ios/web/public/js_messaging/resources/gcrweb.js';
import {sendTokenizedWebKitMessage} from "//brave/ios/web/js_messaging/resources/utils.js";
import {sendWebKitMessage} from "//ios/web/public/js_messaging/resources/utils.js";

function send() {
  sendTokenizedWebKitMessage('ScriptHandlerName', {'key': 'value'});
}
function sendInvalid() {
  sendWebKitMessage('ScriptHandlerName', {'key': 'value'});
}

const testApi = new CrWebApi('message_handler_token_tests');
testApi.addFunction('sendTokenizedWebKitMessage', send);
testApi.addFunction('sendWebKitMessage', sendInvalid);
gCrWeb.registerApi(testApi);
