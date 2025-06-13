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
#include "content/public/browser/navigation_handle.h"

namespace psst {

class PsstShouldProcessPageCheckerImpl : public PsstShouldProcessPageChecker {
 public:
  PsstShouldProcessPageCheckerImpl() = default;
  ~PsstShouldProcessPageCheckerImpl() override = default;
  PsstShouldProcessPageCheckerImpl(const PsstShouldProcessPageCheckerImpl&) =
      delete;
  PsstShouldProcessPageCheckerImpl& operator=(
      const PsstShouldProcessPageCheckerImpl&) = delete;

  bool ShouldProcess(const content::NavigationHandle* handle) const override {
    if (!handle->IsInPrimaryMainFrame() || !handle->HasCommitted() ||
        handle->IsSameDocument()) {
      return false;
    }

    // We only want to process pages that are not restored.
    return handle->GetRestoreType() == content::RestoreType::kNotRestored;
  }
};

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    content::WebContents* contents,
    content::BrowserContext* browser_context,
    const int32_t world_id) {
  CHECK(contents);
  CHECK(browser_context);

  if (browser_context->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    return nullptr;
  }

  auto* prefs = user_prefs::UserPrefs::Get(browser_context);
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

void PsstTabWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* handle) {
  should_process_ = page_checker_->ShouldProcess(handle);
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabWebContentsObserver);

}  // namespace psst
