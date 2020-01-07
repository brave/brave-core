/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

'use strict';

var callbackList = {}; /* message name to callback function */
var sync = 'cfa95630-88fb-4753-8b47-f1ef1819ab58'

if (!self.chrome || !self.chrome.ipc) {
    self.chrome = {};
    var ipc = {};

    ipc.once = function (message, cb) {
        callbackList[message] = cb;
        injectedObject.handleMessage(message, '0', '0', '', false);
    };

    ipc.on = ipc.once;

    ipc.send = function (message, arg1, arg2, arg3, arg4) {
        var arg2ToPass = arg2;
        if (undefined != arg2 && typeof arg2 != 'string' && 'save-init-data' != message) {
            arg2ToPass = JSON.stringify(arg2);
        }
        injectedObject.handleMessage(message, undefined != arg1 ? arg1.toString() : '', undefined != arg2ToPass ? arg2ToPass.toString() : arg2ToPass, undefined != arg3 ? arg3.toString() : '', undefined != arg4 ? arg4 : false);
    };

    self.chrome.ipc = ipc;
    chrome.ipcRenderer = chrome.ipc;
}
