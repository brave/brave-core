// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"

namespace psst {

// static
std::unique_ptr<PsstTabWebContentsObserver>
PsstTabWebContentsObserver::MaybeCreateForWebContents(
    content::WebContents* contents,
    PrefService* prefs,
    std::unique_ptr<PsstDialogDelegate> delegate,
    const int32_t world_id) {
  CHECK(prefs);

  return base::WrapUnique<PsstTabWebContentsObserver>(
      new PsstTabWebContentsObserver(contents, prefs,
                                     std::move(delegate), world_id));
}

PsstTabWebContentsObserver::PsstTabWebContentsObserver(
    content::WebContents* web_contents,
    PrefService* prefs,
    std::unique_ptr<PsstDialogDelegate> delegate,
    const int32_t world_id)
    : WebContentsObserver(web_contents),
      script_handler_(std::make_unique<PsstScriptsHandlerImpl>(
          std::move(delegate),
          prefs,
          web_contents,
          web_contents->GetPrimaryMainFrame(),
          world_id)),
      prefs_(prefs) {}

PsstTabWebContentsObserver::~PsstTabWebContentsObserver() = default;

PsstDialogDelegate* PsstTabWebContentsObserver::GetPsstDialogDelegate() const {
  return script_handler_->GetPsstDialogDelegate();
}

void PsstTabWebContentsObserver::PrimaryPageChanged(content::Page& page) {
  should_process_ = !page.GetMainDocument()
                         .GetController()
                         .GetLastCommittedEntry()
                         ->IsRestored();
}

void PsstTabWebContentsObserver::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!IsPsstEnabled(prefs_)) {
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
