/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_CONTENTS_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_CONTENTS_OBSERVER_H_

#define GetOrCreateSession                          \
  GetOrCreateSession_ChromiumImpl(                  \
      content::NavigationHandle* navigation_handle, \
      content::WebContents* opener) const;          \
  scoped_refptr<MediaEngagementSession> GetOrCreateSession

#include "src/chrome/browser/media/media_engagement_contents_observer.h"  // IWYU pragma: export

#undef GetOrCreateSession

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_CONTENTS_OBSERVER_H_
