// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_

#include <cstddef>
#include <optional>

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/browser/core/psst_settings_service.h"
#include "brave/components/psst/common/psst_script_responses.h"
#include "brave/components/psst/common/psst_ui_common.mojom-shared.h"
#include "brave/components/psst/common/psst_ui_common.mojom.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "url/origin.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class PsstUiPresenter;

class PsstUiDelegateImpl : public PsstTabWebContentsObserver::PsstUiDelegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnSetRequestDone(const std::string& url,
                                  const std::optional<std::string>& error) {}
    virtual void OnSetCompleted(
        const std::optional<std::vector<std::string>>& applied_checks,
        const std::optional<std::vector<std::string>>& errors) {}
  };

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);
  bool HasObserver(Observer* observer);

  base::WeakPtr<PsstUiDelegateImpl> AsWeakPtr();

  using ConsentCallback =
      base::OnceCallback<void(const std::vector<std::string>& disabled_checks)>;

  explicit PsstUiDelegateImpl(PsstSettingsService* psst_settings_service,
                              std::unique_ptr<PsstUiPresenter> ui_presenter);
  ~PsstUiDelegateImpl() override;

  PsstUiDelegateImpl(const PsstUiDelegateImpl&) = delete;
  PsstUiDelegateImpl& operator=(const PsstUiDelegateImpl&) = delete;

  void OnUserAcceptedPsstSettings(const url::Origin& origin,
                                  base::ListValue urls_to_skip);

  // PsstUiDelegate overrides
  void Show(url::Origin origin,
            PsstWebsiteSettings dialog_data,
            const std::string& site_name,
            base::ListValue tasks,
            ConsentCallback apply_changes_callback) override;

  void UpdateTasks(long progress,
                   const std::vector<PolicyTask>& applied_tasks,
                   const mojom::PsstStatus status) override;

  std::optional<PsstWebsiteSettings> GetPsstWebsiteSettings(
      const url::Origin& origin,
      const std::string& user_id) override;

 private:
  void OnUserAcceptedInfobar(const url::Origin& origin, const bool is_accepted);

  std::unique_ptr<PsstUiPresenter> ui_presenter_;
  std::optional<PsstWebsiteSettings> dialog_data_;
  std::optional<url::Origin> origin_;
  std::optional<base::ListValue> tasks_;
  ConsentCallback apply_changes_callback_;
  raw_ptr<PsstSettingsService> psst_settings_service_ = nullptr;
  base::ObserverList<Observer> observer_list_;
  base::WeakPtrFactory<PsstUiDelegateImpl> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
