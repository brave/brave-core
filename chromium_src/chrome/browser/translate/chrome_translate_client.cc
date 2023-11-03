/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"

#define BRAVE_P3A_REGISTER_TRANSLATE_METRICS    \
  translate_manager_->RegisterTranslateMetrics( \
      g_brave_browser_process->process_misc_metrics()->translate_metrics());

#include "src/chrome/browser/translate/chrome_translate_client.cc"

#undef BRAVE_P3A_REGISTER_TRANSLATE_METRICS
