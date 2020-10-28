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
#include "extensions/browser/extension_registry.h"

namespace ipfs {

constexpr size_t kP3ATimerInterval = 5;

// IPFS companion installed?
// i) No, ii) Yes
void RecordIPFSCompanionInstalled(content::BrowserContext* context) {
  const char ipfs_companion_extension_id[] = "nibjojkomfdiaoajekhjakgkdhaomnch";
  auto* registry = extensions::ExtensionRegistry::Get(context);
  bool installed =
      registry->enabled_extensions().Contains(ipfs_companion_extension_id);
  UMA_HISTOGRAM_BOOLEAN("Brave.IPFS.IPFSCompanionInstalled", installed);
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
void RecordIPFSDaemonRunTime(ipfs::IpfsService* service) {
  if (!service->GetDaemonStartTime().is_null()) {
    UMA_HISTOGRAM_TIMES("Brave.IPFS.DaemonRunTime",
                        base::TimeTicks::Now() - service->GetDaemonStartTime());
  }
}

IpfsP3A::IpfsP3A(IpfsService* service, content::BrowserContext* context)
    : service_(service), context_(context) {
  RecordInitialIPFSP3AState();
  timer_.Start(FROM_HERE, base::TimeDelta::FromMinutes(kP3ATimerInterval),
               base::Bind(&IpfsP3A::RecordDaemonUsage, base::Unretained(this)));
}

void IpfsP3A::RecordInitialIPFSP3AState() {
  auto* prefs = user_prefs::UserPrefs::Get(context_);
  RecordIPFSCompanionInstalled(context_);
  RecordIPFSDetectionPromptCount(prefs);
  RecordIPFSGatewaySetting(prefs);
}

void IpfsP3A::RecordDaemonUsage() {
  RecordIPFSDaemonRunTime(service_);
}

void IpfsP3A::Stop() {
  timer_.Stop();
}

}  // namespace ipfs
