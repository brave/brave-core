/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_extra_parts_p3a.h"

#include "base/task/task_traits.h"
#include "brave/browser/updater/features.h"
#include "brave/browser/updater/updater_p3a.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/browser/browser_thread.h"

BraveBrowserMainExtraPartsP3A::BraveBrowserMainExtraPartsP3A() = default;

BraveBrowserMainExtraPartsP3A::~BraveBrowserMainExtraPartsP3A() = default;

void BraveBrowserMainExtraPartsP3A::PostBrowserStart() {
  content::GetUIThreadTaskRunner({base::TaskPriority::BEST_EFFORT})
      ->PostTask(FROM_HERE,
                 base::BindOnce(&BraveBrowserMainExtraPartsP3A::ReportLaunch,
                                weak_ptr_factory_.GetWeakPtr()));
}

void BraveBrowserMainExtraPartsP3A::ReportLaunch() {
  if (g_browser_process && !g_browser_process->IsShuttingDown()) {
    brave_updater::ReportLaunch(base::Time::Now(), chrome::kChromeVersion,
                                brave_updater::ShouldUseOmaha4(),
                                g_browser_process->local_state());
  }
  was_post_browser_start_called_ = true;
}
