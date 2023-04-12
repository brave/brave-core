/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

var iframe = document.createElement('IFRAME');
iframe.id = iframe.name = 'test_iframe';
iframe.src = 'about:blank';
document.body.appendChild(iframe);

var frame = window.frames['test_iframe'];
frame.document.open();
frame.document.write('<script>console.log("message from frame:", document.location.href)</script>');
frame.document.close();
