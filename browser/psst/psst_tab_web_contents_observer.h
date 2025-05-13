// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_BROWSER_PSST_PSST_TAB_WEB_CONTENTS_OBSERVER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefService;

namespace psst {

class PsstDialogDelegate;
class PsstScriptsHandler;
class PsstRuleRegistry;

FORWARD_DECLARE_TEST(PsstTabWebContentsObserverBrowserTest,
                     RuleMatchTestScriptTrue);

class PsstTabWebContentsObserver : public content::WebContentsObserver {
 public:
  static std::unique_ptr<PsstTabWebContentsObserver> MaybeCreateForWebContents(
      content::WebContents* contents,
      std::unique_ptr<PsstDialogDelegate> delegate,
      const int32_t world_id);

  ~PsstTabWebContentsObserver() override;
  PsstTabWebContentsObserver(const PsstTabWebContentsObserver&) = delete;
  PsstTabWebContentsObserver& operator=(const PsstTabWebContentsObserver&) =
      delete;

  PsstDialogDelegate* GetPsstDialogDelegate() const;

 private:
  PsstTabWebContentsObserver(content::WebContents*,
                             std::unique_ptr<PsstDialogDelegate> delegate,
                             const int32_t world_id);

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  void SetScriptHandlerForTesting(
      std::unique_ptr<PsstScriptsHandler> script_handler);

  std::unique_ptr<PsstScriptsHandler> script_handler_;
  const raw_ptr<PrefService> prefs_;
  const raw_ptr<PsstRuleRegistry> psst_rule_registry_;
  bool should_process_ = false;

  FRIEND_TEST_ALL_PREFIXES(PsstTabWebContentsObserverBrowserTest,
                           RuleMatchTestScriptTrue);
  friend class PsstTabWebContentsObserverBrowserTest;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_TAB_WEB_CONTENTS_OBSERVER_H_
