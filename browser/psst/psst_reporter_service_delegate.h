// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_REPORTER_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_PSST_PSST_REPORTER_SERVICE_DELEGATE_H_

#include "brave/components/psst/core/browser/psst_reporter_service.h"
#include "components/component_updater/component_updater_service.h"

namespace psst {

class PsstReporterServiceDelegate : public PsstReporterService::Delegate {
 public:
  explicit PsstReporterServiceDelegate(
      component_updater::ComponentUpdateService* component_update_service);
  PsstReporterServiceDelegate(const PsstReporterServiceDelegate&) = delete;
  PsstReporterServiceDelegate& operator=(const PsstReporterServiceDelegate&) =
      delete;
  ~PsstReporterServiceDelegate() override;

  std::vector<psst::PsstReporterService::PsstComponentInfo> GetComponentInfos()
      const override;
  std::string GetChannelName() const override;

 private:
  const raw_ptr<component_updater::ComponentUpdateService>
      component_update_service_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_REPORTER_SERVICE_DELEGATE_H_
