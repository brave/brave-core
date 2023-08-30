/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SITE_ENGAGEMENT_CONTENT_SITE_ENGAGEMENT_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SITE_ENGAGEMENT_CONTENT_SITE_ENGAGEMENT_SERVICE_H_

#define HandleMediaPlaying                                           \
  UnusedFunction() {}                                                \
                                                                     \
  friend class ::site_engagement::HistoryAwareSiteEngagementService; \
  virtual void HandleMediaPlaying

#define HandleNavigation \
  UnusedFunction2() {}   \
  virtual void HandleNavigation

#define HandleUserInput \
  UnusedFunction3() {}  \
  virtual void HandleUserInput

namespace site_engagement {
class HistoryAwareSiteEngagementService;
}

#include "src/components/site_engagement/content/site_engagement_service.h"  // IWYU pragma: export

#undef HandleMediaPlaying
#undef HandleNavigation
#undef HandleUserInput

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SITE_ENGAGEMENT_CONTENT_SITE_ENGAGEMENT_SERVICE_H_
