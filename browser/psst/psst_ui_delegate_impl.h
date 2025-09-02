// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/common/psst_script_responses.h"

namespace psst {

class PsstUiDelegateImpl : public PsstTabWebContentsObserver::PsstUiDelegate {
 public:
  PsstUiDelegateImpl();
  ~PsstUiDelegateImpl() override;

  // PsstUiDelegate overrides
  void SetAppliedItems(long progress,
                       const std::vector<PsstTask>& applied_tasks) override;
  void SetComplete() override;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
