// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "brave/components/psst/browser/content/psst_scripts_handler_impl.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"

namespace psst {

bool ShouldProcess(const content::NavigationHandle* handle) {
  if (!handle->IsInPrimaryMainFrame() || !handle->HasCommitted() ||
      handle->IsSameDocument() ||
      handle->GetRestoreType() == content::RestoreType::kRestored) {
    return false;
  }

  return true;
}

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    content::WebContents* contents,
    content::BrowserContext* browser_context,
    PrefService* prefs,
    const int32_t world_id) {
  CHECK(contents);
  CHECK(browser_context);
  CHECK(prefs);

  if (browser_context->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    return nullptr;
  }

  return base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(
          contents, prefs,
          std::make_unique<PsstScriptsHandlerImpl>(
              prefs, contents, contents->GetPrimaryMainFrame(), world_id)));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PrefService* prefs,
    std::unique_ptr<ScriptsHandler> script_handler)
    : WebContentsObserver(web_contents),
      script_handler_(std::move(script_handler)),
      prefs_(prefs) {}

PsstTabWebContentsObserver::~PsstTabWebContentsObserver() = default;

void PsstTabWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!prefs_->GetBoolean(prefs::kPsstEnabled)) {
    should_process_ = false;
  } else {
    should_process_ = ShouldProcess(handle);
  }
}

void PsstTabWebContentsObserver::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!std::exchange(should_process_, false)) {
    return;
  }

  script_handler_->Start();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabWebContentsObserver);

}  // namespace psst
