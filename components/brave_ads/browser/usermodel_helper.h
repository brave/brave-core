/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sessions/core/session_id.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/navigation_entry.h"

// Have a look here for a list of callbacks
// https://cs.chromium.org/chromium/src/content/public/browser/web_contents_observer.h?q=webcontentsobserver&l=62

namespace brave_ads {

class UsermodelService;

class UserModelHelper : public content::WebContentsObserver,
                        public BrowserListObserver,
                        public content::WebContentsUserData<UserModelHelper> {
 public:
    UserModelHelper(content::WebContents*);
    ~UserModelHelper() override;
    // content::WebContentsObserver
    void TitleWasSet(content::NavigationEntry* entry) override;

    void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                       const GURL& validated_url) override;

    void OnDataReceived(const base::Value* val);

 private:
  SessionID tab_id_;

  UsermodelService* usermodel_service_;  // NOT OWNED
  
  friend class content::WebContentsUserData<UserModelHelper>;
  DISALLOW_COPY_AND_ASSIGN(UserModelHelper);
};
}
