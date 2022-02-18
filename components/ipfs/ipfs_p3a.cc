/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_p3a.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"

namespace ipfs {

constexpr size_t kP3ATimerInterval = 1;

int GetIPFSDetectionPromptBucket(PrefService* prefs) {
  int bucket = 0;
  auto infobar_count = prefs->GetInteger(kIPFSInfobarCount);
  if (infobar_count == 1) {
    bucket = 1;
  } else if (infobar_count >= 2 && infobar_count <= 5) {
    bucket = 2;
  } else if (infobar_count > 5) {
    bucket = 3;
  }
  return bucket;
}

int GetDaemonUsageBucket(base::TimeDelta elapsed_time) {
  if (elapsed_time <= base::Minutes(5)) {
    return 0;
  }
  if (elapsed_time <= base::Hours(1)) {
    return 1;
  }
  if (elapsed_time <= base::Hours(24)) {
    return 2;
  }
  return 3;
}

// How many lifetime times are IPFS detection prompts shown without installing
// i) 0 times, ii) 1, iii) 2-5 times, iv) 5+ times or more?
void RecordIPFSDetectionPromptCount(PrefService* prefs) {
  const int max_bucket = 4;
  int bucket = GetIPFSDetectionPromptBucket(prefs);
  UMA_HISTOGRAM_EXACT_LINEAR(kDetectionPromptCountHistogramName, bucket,
                             max_bucket);
}

// IPFS state
// Ask (0), Gateway (1), Local Node (2), Disabled (3)
void RecordIPFSGatewaySetting(PrefService* prefs) {
  auto resolve_method = prefs->GetInteger(kIPFSResolveMethod);
  UMA_HISTOGRAM_EXACT_LINEAR(kGatewaySettingHistogramName, resolve_method, 4);
}

// Was the IPFS local node installed? If so, is it still used?
// https://github.com/brave/brave-browser/wiki/P3A#q44-was-the-ipfs-local-node-installed-if-so-is-it-still-used
void RecordIPFSLocalNodeRetention(PrefService* prefs) {
  auto resolve_method = static_cast<IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  auto local_node_used = prefs->GetBoolean(kIPFSLocalNodeUsed);
  int bucket = 0;
  switch (resolve_method) {
    case IPFSResolveMethodTypes::IPFS_ASK:
      bucket = local_node_used;
      break;
    case IPFSResolveMethodTypes::IPFS_LOCAL:
      if (!local_node_used)
        prefs->SetBoolean(kIPFSLocalNodeUsed, true);
      bucket = 1;
      break;
    case IPFSResolveMethodTypes::IPFS_GATEWAY:
    case IPFSResolveMethodTypes::IPFS_DISABLED:
      if (local_node_used)
        bucket = 2;
      break;
    default:
      break;
  }
  UMA_HISTOGRAM_EXACT_LINEAR(kLocalNodeRetentionHistogramName, bucket, 3);
}

// How long did the daemon run?
// i) 0-5min, ii) 5-60min, iii) 1h-24h, iv) 24h+?
void RecordIPFSDaemonRunTime(base::TimeDelta elapsed_time) {
  UMA_HISTOGRAM_EXACT_LINEAR(kDaemonRunTimeHistogramName,
                             GetDaemonUsageBucket(elapsed_time), 4);
}

IpfsP3A::IpfsP3A(IpfsService* service, PrefService* pref_service)
    : service_(service), pref_service_(pref_service) {
  // service/pref_service may be null for unit/browser tests
  if (service != nullptr)
    service->AddObserver(this);
  if (pref_service != nullptr) {
    RecordInitialIPFSP3AState();
    pref_change_registrar_.Init(pref_service);
    pref_change_registrar_.Add(
        kIPFSResolveMethod,
        base::BindRepeating(&IpfsP3A::OnIPFSResolveMethodChanged,
                            base::Unretained(this)));
  }
}

IpfsP3A::~IpfsP3A() {
  if (service_ != nullptr)
    service_->RemoveObserver(this);
}

void IpfsP3A::RecordInitialIPFSP3AState() {
  RecordIPFSDetectionPromptCount(pref_service_);
  RecordIPFSGatewaySetting(pref_service_);
  RecordIPFSLocalNodeRetention(pref_service_);
}

void IpfsP3A::RecordDaemonUsage() {
  FlushTimeDelta();
  RecordIPFSDaemonRunTime(elapsed_time_);
}

void IpfsP3A::OnIPFSResolveMethodChanged() {
  RecordIPFSGatewaySetting(pref_service_);
  RecordIPFSLocalNodeRetention(pref_service_);
}

void IpfsP3A::OnIpfsLaunched(bool result, int64_t pid) {
  if (timer_.IsRunning()) {
    timer_.Stop();
  }

  daemon_start_time_ = base::TimeTicks::Now();
  timer_.Start(
      FROM_HERE, base::Minutes(kP3ATimerInterval),
      base::BindRepeating(&IpfsP3A::RecordDaemonUsage, base::Unretained(this)));
}

void IpfsP3A::OnIpfsShutdown() {
  timer_.Stop();
  FlushTimeDelta();
  daemon_start_time_ = base::TimeTicks();
  RecordDaemonUsage();
}

void IpfsP3A::FlushTimeDelta() {
  if (!daemon_start_time_.is_null()) {
    elapsed_time_ += base::TimeTicks::Now() - daemon_start_time_;
    daemon_start_time_ = base::TimeTicks::Now();
  }
}

}  // namespace ipfs
