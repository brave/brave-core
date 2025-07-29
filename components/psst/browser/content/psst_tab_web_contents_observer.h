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
#include "content/public/browser/web_contents_observer.h"

class PrefService;

namespace psst {

class MatchedRule;
class PsstRuleRegistry;

class PsstTabWebContentsObserver : public content::WebContentsObserver {
 public:
  using InsertScriptInPageCallback = base::OnceCallback<void(base::Value)>;
  class ScriptsHandler {
   public:
    virtual ~ScriptsHandler() = default;
    virtual void InsertScriptInPage(const std::string& script,
                                    InsertScriptInPageCallback cb) = 0;
  };

  static std::unique_ptr<PsstTabWebContentsObserver> MaybeCreateForWebContents(
      content::WebContents* contents,
      content::BrowserContext* browser_context,
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
                             std::unique_ptr<ScriptsHandler> script_handler);

  bool ShouldInsertScriptForPage(int id);
  void InsertUserScript(int id, std::unique_ptr<MatchedRule> rule);

  void OnUserScriptResult(int id,
                          const std::string& policy_script,
                          base::Value script_result);

  // content::WebContentsObserver overrides
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  const raw_ptr<PsstRuleRegistry> registry_;
  const raw_ptr<PrefService> prefs_;
  std::unique_ptr<ScriptsHandler> script_handler_;

  base::WeakPtrFactory<PsstTabWebContentsObserver> weak_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
