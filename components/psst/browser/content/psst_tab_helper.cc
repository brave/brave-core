// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_helper.h"

#include <cstddef>
#include <memory>
#include <utility>

#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"

namespace psst {

// static
std::unique_ptr<PsstTabHelper> PsstTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents,
    std::unique_ptr<PsstDialogDelegate> delegate) {
  if (contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    return nullptr;
  }

  return base::WrapUnique<PsstTabHelper>(new PsstTabHelper(
      contents, std::move(delegate), ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

PsstTabHelper::PsstTabHelper(content::WebContents* web_contents,
                             std::unique_ptr<PsstDialogDelegate> delegate,
                             const int32_t world_id)
    : WebContentsObserver(web_contents),
      script_handler_(std::make_unique<PsstScriptsHandlerImpl>(
          std::move(delegate),
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()),
          web_contents,
          web_contents->GetPrimaryMainFrame())),
      prefs_(user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {}

PsstTabHelper::~PsstTabHelper() = default;

PsstDialogDelegate* PsstTabHelper::GetPsstDialogDelegate() const {
  return script_handler_->GetPsstDialogDelegate();
}

void PsstTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  should_process_ =
      navigation_handle->GetRestoreType() == content::RestoreType::kNotRestored;
}

void PsstTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  if (!PsstRuleRegistryAccessor::GetInstance()->Registry() ||
      !GetEnablePsstFlag(prefs_)) {
    return;
  }

  // Make sure it gets reset.
  if (const bool should_process = std::exchange(should_process_, false);
      !should_process) {
    return;
  }

  script_handler_->Start();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstTabHelper);

}  // namespace psst
