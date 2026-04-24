/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Feature MagicStackAndroid was cleaned up at the upstream, this made
// showMagicStack() unconditional, exposing null-access issue in
// home_modules_ranking_helper.cc

#define BRAVE_JNI_HOME_MODULES_RANKING_HELPER_GET_CLASSIFICATION_RESULT \
  if (!registry) {                                                      \
    return;                                                             \
  }
#define BRAVE_JNI_HOME_MODULES_RANKING_HELPER_NOTIFY_CARD_SHOWN \
  if (!registry) {                                              \
    return;                                                     \
  }
#define BRAVE_JNI_HOME_MODULES_RANKING_HELPER_NOTIFY_CARD_INTERACTED \
  if (!registry) {                                                   \
    return;                                                          \
  }

#include <chrome/browser/segmentation_platform/android/home_modules_ranking_helper.cc>

#undef BRAVE_JNI_HOME_MODULES_RANKING_HELPER_NOTIFY_CARD_INTERACTED
#undef BRAVE_JNI_HOME_MODULES_RANKING_HELPER_NOTIFY_CARD_SHOWN
#undef BRAVE_JNI_HOME_MODULES_RANKING_HELPER_GET_CLASSIFICATION_RESULT
