/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ACCOUNT_ALLOW_BRAVE_ACCOUNT_TAG_H_
#define BRAVE_BROWSER_BRAVE_ACCOUNT_ALLOW_BRAVE_ACCOUNT_TAG_H_

#include "content/public/browser/web_contents_user_data.h"

class AllowBraveAccountTag
    : public content::WebContentsUserData<AllowBraveAccountTag> {
 public:
  static void Mark(content::WebContents* web_contents);
  static bool IsSet(content::WebContents* web_contents);

  ~AllowBraveAccountTag() override;

 private:
  friend class content::WebContentsUserData<AllowBraveAccountTag>;
  explicit AllowBraveAccountTag(content::WebContents* web_contents)
      : WebContentsUserData(*web_contents) {}

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_BRAVE_ACCOUNT_ALLOW_BRAVE_ACCOUNT_TAG_H_
