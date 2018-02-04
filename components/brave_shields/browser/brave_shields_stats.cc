/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_stats.h"

#include "content/public/browser/browser_thread.h"

namespace brave_shields {

BraveShieldsStats::BraveShieldsStats() : ads_blocked_(0), trackers_blocked_(0),
    javascript_blocked_(0), https_upgrades_(0), fingerprinting_blocked_(0) {
}

BraveShieldsStats::~BraveShieldsStats() {
}

BraveShieldsStats* BraveShieldsStats::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return base::Singleton<BraveShieldsStats>::get();
}

void BraveShieldsStats::IncrementAdsBlocked() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ads_blocked_++;
}

void BraveShieldsStats::IncrementTrackersBlocked() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  trackers_blocked_++;
}

void BraveShieldsStats::IncrementJavascriptBlocked() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  javascript_blocked_++;
}

void BraveShieldsStats::IncrementHttpsUpgrades() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  https_upgrades_++;
}

void BraveShieldsStats::IncrementFingerprintingBlocked() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  fingerprinting_blocked_++;
}

}  // namespace brave_shields
