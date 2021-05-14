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
#include "content/public/browser/browser_context.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#endif

namespace ipfs {

constexpr size_t kP3ATimerInterval = 1;

// IPFS companion installed?
// i) No, ii) Yes
void RecordIPFSCompanionInstalled(content::BrowserContext* context) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  const char ipfs_companion_extension_id[] = "nibjojkomfdiaoajekhjakgkdhaomnch";
  auto* registry = extensions::ExtensionRegistry::Get(context);
  bool installed =
      registry->enabled_extensions().Contains(ipfs_companion_extension_id);
  UMA_HISTOGRAM_BOOLEAN("Brave.IPFS.IPFSCompanionInstalled", installed);
#endif
}

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
  if (elapsed_time <= base::TimeDelta::FromMinutes(5)) {
    return 0;
  }
  if (elapsed_time <= base::TimeDelta::FromHours(1)) {
    return 1;
  }
  if (elapsed_time <= base::TimeDelta::FromHours(24)) {
    return 2;
  }
  return 3;
}

// How many lifetime times are IPFS detection prompts shown without installing
// i) 0 times, ii) 1, iii) 2-5 times, iv) 5+ times or more?
void RecordIPFSDetectionPromptCount(PrefService* prefs) {
  const int max_bucket = 4;
  int bucket = GetIPFSDetectionPromptBucket(prefs);
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.IPFS.DetectionPromptCount", bucket,
                             max_bucket);
}

// IPFS state
// Ask (0), Gateway (1), Local Node (2), Disabled (3)
void RecordIPFSGatewaySetting(PrefService* prefs) {
  auto resolve_method = prefs->GetInteger(kIPFSResolveMethod);
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.IPFS.GatewaySetting", resolve_method, 4);
}

// How long did the daemon run?
// i) 0-5min, ii) 5-60min, iii) 1h-24h, iv) 24h+?
void RecordIPFSDaemonRunTime(base::TimeDelta elapsed_time) {
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.IPFS.DaemonRunTime",
                             GetDaemonUsageBucket(elapsed_time), 4);
}

IpfsP3A::IpfsP3A(IpfsService* service, content::BrowserContext* context)
    : service_(service), context_(context) {
  RecordInitialIPFSP3AState();
  service->AddObserver(this);
}

IpfsP3A::~IpfsP3A() {
  service_->RemoveObserver(this);
}

void IpfsP3A::RecordInitialIPFSP3AState() {
  auto* prefs = user_prefs::UserPrefs::Get(context_);
  RecordIPFSCompanionInstalled(context_);
  RecordIPFSDetectionPromptCount(prefs);
  RecordIPFSGatewaySetting(prefs);
}

void IpfsP3A::RecordDaemonUsage() {
  FlushTimeDelta();
  RecordIPFSDaemonRunTime(elapsed_time_);
}

void IpfsP3A::OnIpfsLaunched(bool result, int64_t pid) {
  if (timer_.IsRunning()) {
    timer_.Stop();
  }

  daemon_start_time_ = base::TimeTicks::Now();
  timer_.Start(
      FROM_HERE, base::TimeDelta::FromMinutes(kP3ATimerInterval),
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
