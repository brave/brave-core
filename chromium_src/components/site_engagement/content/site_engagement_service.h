/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SITE_ENGAGEMENT_CONTENT_SITE_ENGAGEMENT_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SITE_ENGAGEMENT_CONTENT_SITE_ENGAGEMENT_SERVICE_H_

#define HandleMediaPlaying \
  UnusedFunction() {}      \
                           \
 protected:                \
  void HandleMediaPlaying

#define OnEngagementEvent \
  UnusedFunction2() {}    \
                          \
 private:                 \
  void OnEngagementEvent

#include "src/components/site_engagement/content/site_engagement_service.h"  // IWYU pragma: export

#undef OnEngagementEvent
#undef HandleMediaPlaying

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SITE_ENGAGEMENT_CONTENT_SITE_ENGAGEMENT_SERVICE_H_
