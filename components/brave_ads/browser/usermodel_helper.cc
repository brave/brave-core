/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "usermodel_helper.h"
#include "usermodel_service.h"
#include "usermodel_service_factory.h"

#include "user_profile.h"

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

void brave_ads::UserModelHelper::Classify(const std::string& html, const std::string& url) {
  LOG(INFO) << "Start classification";
  auto scores = usermodel_service_->usermodel_.classifyPage(html);

  // update profiles
  std::string profile_json;
  usermodel_service_->usermodel_state_->Get("user_profile", &profile_json);
  auto profile = usermodel::UserProfile::FromJSON(profile_json);
  profile->Update(scores, url);

  auto predicted = usermodel_service_->usermodel_.winningCategory(scores);
  LOG(INFO) << "Predicted class: "  << predicted;
}

void brave_ads::UserModelHelper::OnDataReceived(const std::string& url, const base::Value* val) {
  std::string html;
  val->GetAsString(&html);
  base::PostTask(FROM_HERE, 
                 base::BindOnce(&brave_ads::UserModelHelper::Classify, 
                 base::Unretained(this), 
                 html,
                 url));
}

void brave_ads::UserModelHelper::ClassifyPage(content::RenderFrameHost* render_frame_host, const std::string& url) {
  LOG(INFO) << "Fetching the html";
  std::string js("document.getElementsByTagName('html')[0].innerHTML");
  render_frame_host->ExecuteJavaScript(
    base::UTF8ToUTF16(js), 
    base::Bind(&brave_ads::UserModelHelper::OnDataReceived, base::Unretained(this), url)
  );
}

void brave_ads::UserModelHelper::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                       const GURL& validated_url) {
    this->ClassifyPage(render_frame_host, validated_url.spec());
    LOG(INFO) << "Usermodel: " << validated_url.spec();
}
