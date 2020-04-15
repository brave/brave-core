/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
#define BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_

#include "components/permissions/permission_request.h"

#include "url/gurl.h"

namespace content {
class WebContents;
}

class WidevinePermissionRequest : public permissions::PermissionRequest {
 public:
  explicit WidevinePermissionRequest(content::WebContents* web_contents);
  ~WidevinePermissionRequest() override;

  base::string16 GetExplanatoryMessageText() const;
  void set_dont_ask_widevine_install(bool dont_ask) {
    dont_ask_widevine_install_ = dont_ask;
  }

 private:
  // PermissionRequest overrides:
  permissions::PermissionRequest::IconId GetIconId() const override;
  base::string16 GetMessageTextFragment() const override;
  GURL GetOrigin() const override;
  void PermissionGranted() override;
  void PermissionDenied() override;
  void Cancelled() override;
  void RequestFinished() override;
  permissions::PermissionRequestType GetPermissionRequestType() const override;

  // It's safe to use this raw |web_contents_| because this request is deleted
  // by PermissionManager that is tied with this |web_contents_|.
  content::WebContents* web_contents_;

  bool dont_ask_widevine_install_ = false;

  DISALLOW_COPY_AND_ASSIGN(WidevinePermissionRequest);
};

#endif  // BRAVE_BROWSER_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
