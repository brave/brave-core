/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefService;
class TemplateURLService;

class WebDiscoveryTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebDiscoveryTabHelper> {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);

  ~WebDiscoveryTabHelper() override;

  WebDiscoveryTabHelper(const WebDiscoveryTabHelper&) = delete;
  WebDiscoveryTabHelper& operator=(const WebDiscoveryTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<WebDiscoveryTabHelper>;
  friend class WebDiscoveryDialogTest;

  explicit WebDiscoveryTabHelper(content::WebContents* contents);

  // content::WebContentsObserver overrides:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  bool NeedVisitCountHandling(PrefService* prefs,
                              TemplateURLService* template_service);
  bool IsBraveSearchDefault(TemplateURLService* template_service);
  bool ShouldShowWebDiscoveryDialog(PrefService* prefs);
  void IncreaseBraveSearchVisitCount(PrefService* prefs);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
