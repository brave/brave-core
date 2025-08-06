/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/default_browser_monitor.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/browser/brave_stats/first_run_util.h"
#include "chrome/browser/shell_integration.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kRegularCheckInterval = base::Hours(3);
constexpr base::TimeDelta kFirstRunDelay = base::Minutes(5);
constexpr base::TimeDelta kSubsequentStartupDelay = base::Minutes(1);

bool GetDefaultBrowserAsBool() {
  shell_integration::DefaultWebClientState state =
      shell_integration::GetDefaultBrowser();
  return state == shell_integration::IS_DEFAULT ||
         state == shell_integration::OTHER_MODE_IS_DEFAULT;
}

}  // namespace

DefaultBrowserMonitor::DefaultBrowserMonitor(PrefService* local_state)
    : local_state_(local_state),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      get_default_browser_callback_(
          base::BindRepeating(&GetDefaultBrowserAsBool)) {}

DefaultBrowserMonitor::~DefaultBrowserMonitor() = default;

void DefaultBrowserMonitor::Start() {
  base::TimeDelta delay = brave_stats::IsFirstRun(local_state_)
                              ? kFirstRunDelay
                              : kSubsequentStartupDelay;

  timer_.Start(FROM_HERE, base::Time::Now() + delay,
               base::BindOnce(&DefaultBrowserMonitor::CheckDefaultBrowserState,
                              base::Unretained(this)));
}

void DefaultBrowserMonitor::SetGetDefaultBrowserCallbackForTesting(
    base::RepeatingCallback<bool()> callback) {
  get_default_browser_callback_ = std::move(callback);
}

void DefaultBrowserMonitor::CheckDefaultBrowserState() {
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, get_default_browser_callback_,
      base::BindOnce(&DefaultBrowserMonitor::OnDefaultBrowserStateReceived,
                     weak_factory_.GetWeakPtr()));
}

void DefaultBrowserMonitor::OnDefaultBrowserStateReceived(bool is_default) {
  int typical_answer = is_default ? 1 : 0;
  int express_answer = is_default ? 1 : (INT_MAX - 1);

  UMA_HISTOGRAM_EXACT_LINEAR(kDefaultBrowserHistogramName, typical_answer, 2);
  UMA_HISTOGRAM_EXACT_LINEAR(kDefaultBrowserDailyHistogramName, express_answer,
                             2);

  timer_.Start(FROM_HERE, base::Time::Now() + kRegularCheckInterval,
               base::BindOnce(&DefaultBrowserMonitor::CheckDefaultBrowserState,
                              weak_factory_.GetWeakPtr()));
}

}  // namespace misc_metrics
