/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_HELPER_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_HELPER_H_

#include "components/sessions/core/session_id.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/browser/navigation_entry.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"

// Have a look here for a list of callbacks
// https://cs.chromium.org/chromium/src/content/public/browser/web_contents_observer.h?q=webcontentsobserver&l=62

namespace brave_ads {

class UsermodelService;

class UserModelHelper : public content::WebContentsObserver,
                        public BrowserListObserver,
                        public content::WebContentsUserData<UserModelHelper>,
                        public base::SupportsWeakPtr<UsermodelService> {
 public:
    UserModelHelper(content::WebContents*);
    ~UserModelHelper() override;
    
    void TitleWasSet(content::NavigationEntry* entry) override;
    void OnWebContentsFocused(content::RenderWidgetHost* render_widget_host) override;
    void DocumentOnLoadCompletedInMainFrame() override;
    void OnAudioStateChanged(bool audible) override;
    void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                      bool will_cause_resize) override;
 
 private:
  SessionID tab_id_;

  UsermodelService* usermodel_service_;  // NOT OWNED
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  
  friend class content::WebContentsUserData<UserModelHelper>;
  DISALLOW_COPY_AND_ASSIGN(UserModelHelper);
};
}

#endif