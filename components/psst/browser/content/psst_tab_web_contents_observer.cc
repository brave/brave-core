// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "brave/components/psst/browser/content/psst_scripts_handler.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"

namespace psst {

class PsstShouldProcessPageCheckerImpl : public PsstShouldProcessPageChecker {
 public:
  PsstShouldProcessPageCheckerImpl() = default;
  ~PsstShouldProcessPageCheckerImpl() override = default;
  PsstShouldProcessPageCheckerImpl(const PsstShouldProcessPageCheckerImpl&) =
      delete;
  PsstShouldProcessPageCheckerImpl& operator=(
      const PsstShouldProcessPageCheckerImpl&) = delete;

  bool ShouldProcess(content::NavigationEntry* entry) const override {
    return entry && !entry->IsRestored();
  }
};

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::CreateForWebContents(content::WebContents* contents,
                                                 PrefService* prefs,
                                                 const int32_t world_id) {
  CHECK(contents);
  CHECK(prefs);

  return base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(
          contents, prefs, std::make_unique<PsstShouldProcessPageCheckerImpl>(),
          PsstScriptsHandler::Create(contents, prefs, world_id)));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PrefService* prefs,
    std::unique_ptr<PsstShouldProcessPageChecker> page_checker,
    std::unique_ptr<PsstScriptsHandler> script_handler)
    : WebContentsObserver(web_contents),
      page_checker_(std::move(page_checker)),
      script_handler_(std::move(script_handler)),
      prefs_(prefs) {}

PsstTabWebContentsObserver::~PsstTabWebContentsObserver() = default;

void PsstTabWebContentsObserver::PrimaryPageChanged(content::Page& page) {
  // Continue to process if the page is not restored.
  should_process_ = page_checker_->ShouldProcess(
      page.GetMainDocument().GetController().GetLastCommittedEntry());
}

void PsstTabWebContentsObserver::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!prefs_->GetBoolean(prefs::kPsstEnabled)) {
    return;
  }

  if (!std::exchange(should_process_, false)) {
    return;
  }

  script_handler_->Start();
}

void PsstTabWebContentsObserver::SetScriptHandlerForTesting(
    std::unique_ptr<PsstScriptsHandler> script_handler) {
  script_handler_ = std::move(script_handler);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabWebContentsObserver);

}  // namespace psst
