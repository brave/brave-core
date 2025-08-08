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
#include "base/values.h"
#include "brave/components/psst/common/prefs.h"
#include "content/public/browser/web_contents_observer.h"

class PrefService;

namespace psst {

class MatchedRule;
class PsstRuleRegistry;
class PsstDialogDelegate;

class PsstTabWebContentsObserver : public content::WebContentsObserver {
 public:
  using InsertScriptInPageCallback = base::OnceCallback<void(base::Value)>;
  class ScriptsInserter {
   public:
    virtual ~ScriptsInserter() = default;
    virtual void InsertScriptInPage(const std::string& script,
                                    std::optional<base::Value> value,
                                    InsertScriptInPageCallback cb) = 0;
  };

  class PsstDialogDelegate {
   public:
    PsstDialogDelegate();
    virtual ~PsstDialogDelegate();

    virtual void SetProgress(const double value) {}
    virtual void SetCompleted() {}
    virtual void Show() {}
    virtual void Close() {}
  };

  static std::unique_ptr<PsstTabWebContentsObserver> MaybeCreateForWebContents(
      content::WebContents* contents,
      content::BrowserContext* browser_context,
      std::unique_ptr<PsstDialogDelegate> delegate,
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
                             std::unique_ptr<ScriptsInserter> script_handler,
                             std::unique_ptr<PsstDialogDelegate> delegate);

  bool ShouldInsertScriptForPage(int id);
  void InsertUserScript(int id, std::unique_ptr<MatchedRule> rule);

  void OnUserScriptResult(int id,
                          std::unique_ptr<MatchedRule> rule,
                          base::Value script_result);
  void OnPolicyScriptResult(int nav_entry_id,
                            std::unique_ptr<MatchedRule> rule,
                            base::Value script_result);

  // content::WebContentsObserver overrides
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  bool ShouldContinueSilently(const MatchedRule& rule,
                              const std::string& user_id);
  void OnUserDialogAction(int nav_entry_id,
                          const bool is_initial,
                          const std::string& user_id,
                          std::unique_ptr<MatchedRule> rule,
                          std::optional<base::Value> script_params,
                          const ConsentStatus status,
                          std::optional<base::Value::List> disabled_checks);

  const raw_ptr<PsstRuleRegistry> registry_;
  const raw_ptr<PrefService> prefs_;
  std::unique_ptr<ScriptsInserter> script_inserter_;
  std::unique_ptr<PsstDialogDelegate> delegate_;

  base::WeakPtrFactory<PsstTabWebContentsObserver> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
