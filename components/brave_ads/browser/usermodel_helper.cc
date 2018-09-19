/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "usermodel_helper.h"
#include "usermodel_service.h"
#include "usermodel_service_factory.h"

#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/navigation_entry.h"
#include "chrome/browser/profiles/profile.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"

using namespace std::placeholders;


brave_ads::UserModelHelper::UserModelHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      tab_id_(SessionTabHelper::IdForTab(web_contents)) {
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

void brave_ads::UserModelHelper::OnDataReceived(const base::Value* val) {
  std::string html;
  val->GetAsString(&html);

  // classify time
  auto scores = usermodel_service_->usermodel_.classifyPage(html);
  auto predicted = usermodel_service_->usermodel_.winningCategory(scores);
  LOG(INFO) << "Predicted class: "  << predicted;
}

void brave_ads::UserModelHelper::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                       const GURL& validated_url) {


    //TODO: this needs to run on taskrunner
    
    std::string js("document.getElementsByTagName('html')[0].innerHTML");
    render_frame_host->ExecuteJavaScript(
      base::UTF8ToUTF16(js), 
      base::Bind(&brave_ads::UserModelHelper::OnDataReceived, base::Unretained(this))
    );

    LOG(INFO) << "Usermodel: " << validated_url.spec();
}
