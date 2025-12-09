// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_

#include <cstddef>
#include <optional>

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/common/psst_script_responses.h"
#include "brave/components/psst/common/psst_ui_common.mojom-shared.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class PsstUiDelegateImpl : public PsstTabWebContentsObserver::PsstUiDelegate {
 public:
  explicit PsstUiDelegateImpl(
      HostContentSettingsMap* host_content_settings_map);
  ~PsstUiDelegateImpl() override;

  PsstUiDelegateImpl(const PsstUiDelegateImpl&) = delete;
  PsstUiDelegateImpl& operator=(const PsstUiDelegateImpl&) = delete;

  void Show(const url::Origin& origin,
            PsstWebsiteSettings dialog_data,
            PsstTabWebContentsObserver::ConsentCallback apply_changes_callback)
      override;

  // PsstUiDelegate overrides
  void UpdateTasks(long progress,
                   const std::vector<PolicyTask>& applied_tasks,
                   const mojom::PsstStatus status) override;

  std::optional<PsstWebsiteSettings> GetPsstWebsiteSettings(
      const url::Origin& origin,
      const std::string& user_id) override;

 private:
  void OnUserAcceptedPsstSettings(const url::Origin& origin,
                                  base::Value::List urls_to_skip);

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  std::optional<PsstWebsiteSettings> dialog_data_;
  PsstTabWebContentsObserver::ConsentCallback apply_changes_callback_;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
