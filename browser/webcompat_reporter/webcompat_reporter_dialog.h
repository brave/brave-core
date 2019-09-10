/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_DIALOG_H_
#define BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_DIALOG_H_

namespace content {
class WebContents;
}

void OpenWebcompatReporterDialog(content::WebContents* initiator);

#endif  // BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_DIALOG_H_
