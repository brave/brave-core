/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_IMPL_H_
#define BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "ios/chrome/browser/application_context_impl.h"

class BrowserPolicyCOnnectorIOS;
class SafeBrowsingService;

namespace base {
class CommandLine;
class SequencedTaskRunner;
}

namespace gcm {
class GCMDriver;
}

namespace metrics_services_manager {
class MetricsServicesManager;
}

class BraveApplicationContextImpl : public ApplicationContextImpl {
 public:
  BraveApplicationContextImpl(
      base::SequencedTaskRunner* local_state_task_runner,
      const base::CommandLine& command_line,
      const std::string& locale);
  ~BraveApplicationContextImpl() override;

  // ApplicationContext implementation.
  void OnAppEnterForeground() override;
  metrics_services_manager::MetricsServicesManager* GetMetricsServicesManager()
      override;
  gcm::GCMDriver* GetGCMDriver() override;
  SafeBrowsingService* GetSafeBrowsingService() override;
  BrowserPolicyConnectorIOS* GetBrowserPolicyConnector() override;

 private:
  std::unique_ptr<metrics_services_manager::MetricsServicesManager>
      metrics_services_manager_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(BraveApplicationContextImpl);
};

#endif  // BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_IMPL_H_
