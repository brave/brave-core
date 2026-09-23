// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_BROWSER_PSST_PSST_TAB_WEB_CONTENTS_OBSERVER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/psst/core/browser/psst_settings_service.h"
#include "brave/components/psst/core/common/psst_metadata_schema.h"
#include "brave/components/psst/core/common/psst_script_responses.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom-shared.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace variations {
class VariationsService;
}

namespace psst {

class MatchedRule;
class PsstRuleRegistry;

class PsstTabWebContentsObserver : public tabs::ContentsObservingTabFeature,
                                   public PsstSettingsService::PrefObserver {
 public:
  using InsertScriptInPageCallback = base::OnceCallback<void(base::Value)>;
  using InsertScriptInPageTimeoutCallback =
      base::RepeatingCallback<void(const int)>;
  using InjectScriptCallback = base::RepeatingCallback<void(
      const std::string&,
      PsstTabWebContentsObserver::InsertScriptInPageCallback)>;
  using InjectScriptAsyncCallback = base::RepeatingCallback<void(
      const std::string&,
      PsstTabWebContentsObserver::InsertScriptInPageCallback)>;
  using ConsentCallback =
      base::OnceCallback<void(const std::vector<std::string>&)>;

  // Delegate interface for UI-related actions. This class is responsible for
  // facilitating communication with the consent dialog, ensuring that the UI
  // reflects the current state accurately.
  class PsstUiDelegate {
   public:
    virtual ~PsstUiDelegate() = default;
    // Show the consent dialog to the user with the provided data.
    virtual void Show(url::Origin origin,
                      PsstWebsiteSettings dialog_data,
                      const int rule_version,
                      std::optional<UserScriptResult> user_script_result,
                      ConsentCallback apply_changes_callback) = 0;
    // Update the UI state based on the applied tasks and progress.
    virtual void UpdateTasks(long progress,
                             const std::vector<PolicyTask>& applied_tasks,
                             const mojom::PsstStatus status) = 0;
    // Provides an access to the PSST settings object
    virtual std::optional<PsstWebsiteSettings> GetPsstWebsiteSettings(
        const url::Origin& origin,
        const std::string& user_id) = 0;
  };

  // Creates an observer for `tab`'s web contents, or returns null for
  // incognito/guest profiles or when PSST is disabled. The returned observer
  // keeps a reference to `tab` for its lifetime. `browser_context`,
  // `psst_settings_service`, and `ui_delegate` must not be null.
  // `variations_service` may be null, in which case the country used for
  // rule matching is left empty. `world_id` is the isolated world that PSST
  // scripts are injected into.
  static std::unique_ptr<PsstTabWebContentsObserver> MaybeCreateForWebContents(
      tabs::TabInterface& tab,
      content::BrowserContext* browser_context,
      std::unique_ptr<PsstUiDelegate> ui_delegate,
      PsstSettingsService* psst_settings_service,
      variations::VariationsService* variations_service,
      const int32_t world_id);

  ~PsstTabWebContentsObserver() override;
  PsstTabWebContentsObserver(const PsstTabWebContentsObserver&) = delete;
  PsstTabWebContentsObserver& operator=(const PsstTabWebContentsObserver&) =
      delete;

  PsstUiDelegate* GetPsstUiDelegate() const;
  base::WeakPtr<PsstTabWebContentsObserver> AsWeakPtr();

 private:
  friend class PsstTabWebContentsObserverUnitTestBase;

  PsstTabWebContentsObserver(tabs::TabInterface& tab,
                             PsstRuleRegistry* registry,
                             PsstSettingsService* psst_settings_service,
                             variations::VariationsService* variations_service,
                             std::unique_ptr<PsstUiDelegate> ui_delegate);

  void InsertUserScript(std::unique_ptr<MatchedRule> rule);

  void OnUserScriptResult(std::unique_ptr<MatchedRule> rule,
                          base::Value script_result);
  void OnUserAcceptedPsstSettings(
      bool is_initial,
      std::unique_ptr<MatchedRule> rule,
      base::Value user_script_result,
      const std::vector<std::string>& perform_for_uids);
  void OnPolicyScriptResult(base::Value script_result);
  void RunWithTimeout(const std::string& script,
                      bool is_async,
                      InsertScriptInPageCallback callback);
  void OnScriptTimeout();

  // content::WebContentsObserver overrides
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void PrimaryPageChanged(content::Page& page) override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  void SetInjectScriptCallback(InjectScriptCallback inject_script_callback);
  void SetInjectAsyncScriptCallback(
      InjectScriptAsyncCallback inject_async_script_callback);
  void CancelInFlightFlow();

  // PsstSettingsService::Observer
  void OnPsstEnableChange(bool new_value) override;

  const raw_ptr<PsstRuleRegistry> registry_;
  const raw_ptr<PsstSettingsService> psst_settings_service_ = nullptr;
  const raw_ptr<variations::VariationsService> variations_service_ = nullptr;
  InjectScriptCallback inject_script_callback_;
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  InjectScriptAsyncCallback inject_async_script_callback_;
  std::unique_ptr<PsstUiDelegate> ui_delegate_;
  base::OneShotTimer timeout_timer_;

  // Whether the currently committed primary page is eligible for PSST
  // processing. Recomputed on every cross-document primary main frame commit.
  bool should_process_current_page_ = false;

  // Weak pointers scoped to the lifetime of the current primary document. Every
  // step of the PSST flow (rule lookup, script results, consent callback,
  // timeout) is bound to this factory, which is invalidated whenever the
  // document is replaced. That makes stale callbacks no-ops by construction, so
  // no step has to validate that its page is still current.
  base::WeakPtrFactory<PsstTabWebContentsObserver> page_weak_factory_{this};

  base::WeakPtrFactory<PsstTabWebContentsObserver> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
