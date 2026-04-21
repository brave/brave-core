/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CONTENT_WEBCAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_WEBCAT_CONTENT_WEBCAT_TAB_HELPER_H_

#include <map>
#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/webcat/core/bundle_parser.h"
#include "brave/components/webcat/core/constants.h"
#include "brave/components/webcat/core/origin_state.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/origin.h"

namespace webcat {

class WebcatTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebcatTabHelper> {
 public:
  ~WebcatTabHelper() override;

  WebcatTabHelper(const WebcatTabHelper&) = delete;
  WebcatTabHelper& operator=(const WebcatTabHelper&) = delete;

  OriginState GetOriginState(const url::Origin& origin) const;
  void SetOriginState(const url::Origin& origin, OriginStateData state);

  bool IsOriginVerified(const url::Origin& origin) const;
  const std::optional<Bundle> GetManifest(const url::Origin& origin) const;

  void MarkOriginVerified(const url::Origin& origin, Bundle bundle);
  void MarkOriginFailed(const url::Origin& origin,
                        WebcatError error,
                        const std::string& detail);

  bool should_show_badge() const { return should_show_badge_; }

 private:
  friend class content::WebContentsUserData<WebcatTabHelper>;

  explicit WebcatTabHelper(content::WebContents* web_contents);

  void DidNavigatePrimaryFrame(
      content::NavigationHandle* navigation_handle) override;
  void PrimaryPageChanged(content::Page& page) override;
  void WebContentsDestroyed() override;

  std::map<url::Origin, OriginStateData> origin_states_;
  bool should_show_badge_ = false;

  base::ScopedObservation<content::WebContents,
                          content::WebContentsObserver>
      observation_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CONTENT_WEBCAT_TAB_HELPER_H_