// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_UNIVERSAL_WEB_CONTENTS_OBSERVERS_H_
#define BRAVE_BROWSER_BRAVE_UNIVERSAL_WEB_CONTENTS_OBSERVERS_H_

namespace content {
class WebContents;
}

void AttachBraveUniversalWebContentsObservers(
    content::WebContents* web_contents);

#endif  // BRAVE_BROWSER_BRAVE_UNIVERSAL_WEB_CONTENTS_OBSERVERS_H_
