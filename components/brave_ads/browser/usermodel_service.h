/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "chrome/browser/profiles/profile.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"

#include "user_model.h"
#include "user_profile.h"
#include "usermodel_state.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/navigation_entry.h"
#include "components/sessions/core/session_id.h"

#include "ad_catalog.h"
#include "notification_event_type.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace message_center {
class MessageCenter;
}  // namespace message_center

namespace brave_ads {

class UsermodelService : public KeyedService,
                         public base::SupportsWeakPtr<UsermodelService> {
 public:
    UsermodelService(Profile* profile);
    ~UsermodelService() override;

    void OnModelLoaded(const std::string& data);
    void OnAdsLoaded(const std::string& data);
    void OnUserProfileLoaded(const std::string& data);

    void SaveUsermodelState(const std::string& state);
    void OnUsermodelStateSaved(bool success);

    void UpdateState(const std::string& key, const std::string& value);
    void OnPageVisited(SessionID tab_id, content::RenderFrameHost* render_frame_host, const std::string& url);
    void OnDataReceived(SessionID tab_id, const std::string& url, const base::Value* val);
    void OnTabFocused(SessionID tab_id);
    void Classify(const std::string& html, const std::string& url, SessionID tab_id);

    //void SettingsUpdated(); 
    void OnNotificationEvent(usermodel::NotificationEventType event);

    void ShowAd();

    usermodel::UserModel usermodel_;
    usermodel::AdCatalog ad_catalog_;
    std::unique_ptr<usermodel::UserProfile> user_profile_;
    
    UserModelState* usermodel_state_;

    // bool doNotDisturb = false;
   const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  
    // this counts how many reasons exist 
    // to not disturb the user. E.g. 
    // playing audio and being in fullscreen mode
    // counts for 2 reasons. If you exit fullscreen mode
    // but keep playing audio, then you have 1 reason.
    // A notification can be shown only when reasons == 0
    unsigned int do_not_disturb_reasons_ = 0;
    
 private:

  typedef std::map<SessionID, std::vector<double>> TabCache;
  TabCache tab_cache_; 

   const base::FilePath usermodel_state_path_;
   const base::FilePath taxonomy_model_path_;
   const base::FilePath ads_feed_path_;

  bool initialized_;
  time_t last_focused_timestamp_;
};
}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_ 
