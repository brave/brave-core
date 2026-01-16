/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/default_browser_monitor.h"

#include "base/metrics/histogram_macros.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#endif

namespace misc_metrics {

#if !BUILDFLAG(IS_ANDROID)
namespace {

constexpr base::TimeDelta kRegularCheckInterval = base::Hours(3);
constexpr base::TimeDelta kFirstRunDelay = base::Minutes(5);
constexpr base::TimeDelta kSubsequentStartupDelay = base::Seconds(10);

}  // namespace
#endif

#if BUILDFLAG(IS_ANDROID)
DefaultBrowserMonitor::DefaultBrowserMonitor() = default;
#else
DefaultBrowserMonitor::DefaultBrowserMonitor(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {}
#endif

DefaultBrowserMonitor::~DefaultBrowserMonitor() = default;

#if !BUILDFLAG(IS_ANDROID)
void DefaultBrowserMonitor::Start() {
  auto delay =
      delegate_->IsFirstRun() ? kFirstRunDelay : kSubsequentStartupDelay;

  timer_.Start(FROM_HERE, base::Time::Now() + delay,
               base::BindOnce(&DefaultBrowserMonitor::CheckDefaultBrowserState,
                              base::Unretained(this)));
}

void DefaultBrowserMonitor::CheckDefaultBrowserState() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&Delegate::IsDefaultBrowser,
                     base::Unretained(delegate_.get())),
      base::BindOnce(&DefaultBrowserMonitor::OnDefaultBrowserStateReceived,
                     weak_factory_.GetWeakPtr()));
}
#endif

void DefaultBrowserMonitor::OnDefaultBrowserStateReceived(bool is_default) {
  bool status_changed =
      !cached_default_status_ || *cached_default_status_ != is_default;

  cached_default_status_ = is_default;

  int typical_answer = is_default ? 1 : 0;
  int express_answer = is_default ? 1 : (INT_MAX - 1);

  UMA_HISTOGRAM_EXACT_LINEAR(kDefaultBrowserHistogramName, typical_answer, 2);
  UMA_HISTOGRAM_EXACT_LINEAR(kDefaultBrowserDailyHistogramName, express_answer,
                             2);

  if (status_changed) {
    for (auto& observer : observers_) {
      observer.OnDefaultBrowserStatusChanged();
    }
  }

#if !BUILDFLAG(IS_ANDROID)
  timer_.Start(FROM_HERE, base::Time::Now() + kRegularCheckInterval,
               base::BindOnce(&DefaultBrowserMonitor::CheckDefaultBrowserState,
                              weak_factory_.GetWeakPtr()));
#endif
}

std::optional<bool> DefaultBrowserMonitor::GetCachedDefaultStatus() const {
  return cached_default_status_;
}

void DefaultBrowserMonitor::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DefaultBrowserMonitor::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace misc_metrics
