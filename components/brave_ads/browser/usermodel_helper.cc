/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "usermodel_helper.h"
#include "usermodel_service.h"
#include "usermodel_service_factory.h"

#include "user_profile.h"

#include "content/public/common/isolated_world_ids.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/navigation_entry.h"
#include "chrome/browser/profiles/profile.h"
#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"

using namespace std::placeholders;

brave_ads::UserModelHelper::UserModelHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      tab_id_(SessionTabHelper::IdForTab(web_contents)),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
  if (!tab_id_.is_valid())
    return;

  BrowserList::AddObserver(this);

  Profile* profile = Profile::FromBrowserContext(
      web_contents->GetBrowserContext());
  usermodel_service_ = UsermodelServiceFactory::GetForProfile(profile);
}

brave_ads::UserModelHelper::~UserModelHelper() {
  BrowserList::RemoveObserver(this);
}

// Additional hooks
// - DidToggleFullscreenModeForTab
// - OnAudioStateChanged 
// - MediaStartedPlaying
// - MediaStoppedPlaying
// - MediaEffectivelyFullscreenChanged
void brave_ads::UserModelHelper::TitleWasSet(content::NavigationEntry* entry) {
  LOG(INFO) << "Title: " << entry->GetTitle();
}

void brave_ads::UserModelHelper::OnWebContentsFocused(content::RenderWidgetHost* render_widget_host) {
  usermodel_service_->OnTabFocused(tab_id_);
}

void  brave_ads::UserModelHelper::DocumentOnLoadCompletedInMainFrame() {
  usermodel_service_->OnPageVisited(
    tab_id_, 
    web_contents()->GetMainFrame(), 
    web_contents()->GetVisibleURL().spec());
};

void brave_ads::UserModelHelper::OnAudioStateChanged(bool audible) {
  if (audible) {
    usermodel_service_->do_not_disturb_reasons_++;
  } else {
    if (usermodel_service_->do_not_disturb_reasons_ > 0) {
      usermodel_service_->do_not_disturb_reasons_--;
    }
  }
};

void brave_ads::UserModelHelper::DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                  bool will_cause_resize) {
  if (entered_fullscreen) {
    usermodel_service_->do_not_disturb_reasons_++;
  } else {
    if (usermodel_service_->do_not_disturb_reasons_ > 0) {
      usermodel_service_->do_not_disturb_reasons_--;
    }
  }
};