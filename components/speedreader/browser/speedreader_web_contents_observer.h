/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_BROWSER_SPEEDREADER_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_BROWSER_SPEEDREADER_WEB_CONTENTS_OBSERVER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/strings/string16.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class PrefRegistrySimple;

namespace speedreader {

class SpeedreaderWebContentsObserver : public content::WebContentsObserver,
    public content::WebContentsUserData<SpeedreaderWebContentsObserver> {
 public:
  explicit SpeedreaderWebContentsObserver(content::WebContents*);
  ~SpeedreaderWebContentsObserver() override;

  void DisableSpeedreaderOnce(const std::vector<std::string>& origins,
                        content::WebContents* web_contents);

 protected:
  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* host) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  //Invoked if an IPC message is coming from a specific RenderFrameHost.
  bool OnMessageReceived(const IPC::Message& message,
      content::RenderFrameHost* render_frame_host) override;

  void OnSpeedreaderTransformed(
      content::RenderFrameHost* render_frame_host);

 private:
  friend class content::WebContentsUserData<SpeedreaderWebContentsObserver>;
  std::vector<std::string> disabled_speedreader_origins_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
  DISALLOW_COPY_AND_ASSIGN(SpeedreaderWebContentsObserver);
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_BROWSER_SPEEDREADER_WEB_CONTENTS_OBSERVER_H_
