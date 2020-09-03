/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_application_context_impl.h"

#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "brave/ios/browser/metrics/ios_brave_metrics_services_manager_client.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/policy/browser_policy_connector_ios.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/safe_browsing/safe_browsing_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveApplicationContextImpl::BraveApplicationContextImpl(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line,
    const std::string& locale)
    : ApplicationContextImpl(local_state_task_runner, command_line, locale) {}

BraveApplicationContextImpl::~BraveApplicationContextImpl() {}

void BraveApplicationContextImpl::OnAppEnterForeground() {
  DCHECK(thread_checker_.CalledOnValidThread());

  PrefService* local_state = GetLocalState();
  local_state->SetBoolean(prefs::kLastSessionExitedCleanly, false);
}

metrics_services_manager::MetricsServicesManager*
BraveApplicationContextImpl::GetMetricsServicesManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!metrics_services_manager_) {
    metrics_services_manager_.reset(
        new metrics_services_manager::MetricsServicesManager(
            std::make_unique<IOSBraveMetricsServicesManagerClient>(
                GetLocalState())));
  }
  return metrics_services_manager_.get();
}

gcm::GCMDriver* BraveApplicationContextImpl::GetGCMDriver() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

SafeBrowsingService* BraveApplicationContextImpl::GetSafeBrowsingService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

BrowserPolicyConnectorIOS* BraveApplicationContextImpl::GetBrowserPolicyConnector() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}
