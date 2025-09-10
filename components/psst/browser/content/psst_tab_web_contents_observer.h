// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_WEB_CONTENTS_OBSERVER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/psst/common/psst_script_responses.h"
#include "brave/components/psst/common/psst_common.h"
#include "content/public/browser/web_contents_observer.h"

class PrefService;

namespace psst {

class MatchedRule;
class PsstRuleRegistry;

class PsstTabWebContentsObserver : public content::WebContentsObserver {
 public:
  using InsertScriptInPageCallback = base::OnceCallback<void(base::Value)>;
  using InsertScriptInPageTimeoutCallback =
      base::RepeatingCallback<void(const int)>;
  using InjectScriptCallback = base::RepeatingCallback<void(
      const std::string&,
      PsstTabWebContentsObserver::InsertScriptInPageCallback)>;

  // Delegate interface for UI-related actions. This class is responsible for
  // facilitating communication with the consent dialog, ensuring that the UI
  // reflects the current state accurately.
  class PsstUiDelegate {
   public:
    virtual ~PsstUiDelegate() = default;
    // Update the UI state based on the applied tasks and progress.
    virtual void UpdateTasks(long progress,
                             const std::vector<PolicyTask>& applied_tasks,
                             const PsstStatus status) = 0;
  };

  // Helper class that watches for hanging injected JS scripts and ensures
  // their callbacks eventually fire.
  class TimedScriptRepeatingCallback {
   public:
    explicit TimedScriptRepeatingCallback(
        InjectScriptCallback inject_script_callback,
        InsertScriptInPageTimeoutCallback timeout_callback,
        base::TimeDelta timeout);
    ~TimedScriptRepeatingCallback();

    TimedScriptRepeatingCallback(const TimedScriptRepeatingCallback&) = delete;
    TimedScriptRepeatingCallback& operator=(
        const TimedScriptRepeatingCallback&) = delete;

    // Runs the script injection, setting up the timeout and callback.
    void Run(const int last_committed_entry_id,
             const std::string& script,
             InsertScriptInPageCallback callback);

   private:
    void OnResult(base::Value value);
    void OnTimeout(const int last_committed_entry_id);

    InjectScriptCallback inject_script_callback_;
    base::TimeDelta timeout_delay_;
    base::OneShotTimer timer_;
    InsertScriptInPageCallback result_callback_;
    InsertScriptInPageTimeoutCallback timeout_callback_;
    base::WeakPtrFactory<TimedScriptRepeatingCallback> weak_factory_{this};
  };

  static std::unique_ptr<PsstTabWebContentsObserver> MaybeCreateForWebContents(
      content::WebContents* contents,
      content::BrowserContext* browser_context,
      std::unique_ptr<PsstUiDelegate> ui_delegate,
      PrefService* prefs,
      const int32_t world_id);

  ~PsstTabWebContentsObserver() override;
  PsstTabWebContentsObserver(const PsstTabWebContentsObserver&) = delete;
  PsstTabWebContentsObserver& operator=(const PsstTabWebContentsObserver&) =
      delete;

 private:
  friend class PsstTabWebContentsObserverUnitTestBase;

  PsstTabWebContentsObserver(content::WebContents* web_contents,
                             PsstRuleRegistry* registry,
                             PrefService* prefs,
                             std::unique_ptr<PsstUiDelegate> ui_delegate,
                             InjectScriptCallback inject_script_callback,
                             base::TimeDelta script_execution_timeout);

  bool ShouldInsertScriptForPage(int id);
  void InsertUserScript(int id, std::unique_ptr<MatchedRule> rule);

  void OnUserScriptResult(int id,
                          std::unique_ptr<MatchedRule> rule,
                          base::Value script_result);
  void OnPolicyScriptResult(int nav_entry_id, base::Value script_result);
  void OnScriptTimeout(int id);

  // content::WebContentsObserver overrides
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  const raw_ptr<PsstRuleRegistry> registry_;
  const raw_ptr<PrefService> prefs_;
  std::unique_ptr<TimedScriptRepeatingCallback> inject_script_callback_;
  std::unique_ptr<PsstUiDelegate> ui_delegate_;

  base::WeakPtrFactory<PsstTabWebContentsObserver> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
