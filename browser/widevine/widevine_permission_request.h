/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_

#include "base/gtest_prod_util.h"
#include "components/permissions/permission_request.h"

#include "base/memory/raw_ptr.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

class WidevinePermissionRequest : public permissions::PermissionRequest {
 public:
  WidevinePermissionRequest(content::WebContents* web_contents,
                            bool for_restart);

  WidevinePermissionRequest(const WidevinePermissionRequest&) = delete;
  WidevinePermissionRequest& operator=(const WidevinePermissionRequest&) =
      delete;

  ~WidevinePermissionRequest() override;

  std::u16string GetExplanatoryMessageText() const;
  void set_dont_ask_widevine_install(bool dont_ask) {
    dont_ask_widevine_install_ = dont_ask;
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(WidevinePermissionRequestBrowserTest,
                           TriggerTwoPermissionTest);
  static bool is_test_;

  // PermissionRequest overrides:
  std::u16string GetMessageTextFragment() const override;
  void PermissionDecided(ContentSetting result, bool is_one_time);
  void DeleteRequest();

  // It's safe to use this raw |web_contents_| because this request is deleted
  // by PermissionManager that is tied with this |web_contents_|.
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  bool dont_ask_widevine_install_ = false;

  // Only can be true on linux.
  // On linux, browser will use another permission request buble after finishing
  // installation to ask user about restarting because installed widevine can
  // only be used after re-launch.
  bool for_restart_ = false;
};

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
