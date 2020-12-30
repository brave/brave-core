/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_ONION_LOCATION_TAB_HELPER_H_
#define BRAVE_COMPONENTS_TOR_ONION_LOCATION_TAB_HELPER_H_

#include "content/public/browser/web_contents_user_data.h"

namespace tor {

class OnionLocationTabHelper
    : public content::WebContentsUserData<OnionLocationTabHelper> {
 public:
  ~OnionLocationTabHelper() override;

  static void SetOnionLocation(content::WebContents* web_contents,
                               const GURL& onion_location);

  bool should_show_icon() const { return !onion_location_.is_empty(); }

  GURL onion_location() const { return onion_location_; }

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  explicit OnionLocationTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<OnionLocationTabHelper>;

  GURL onion_location_;

  OnionLocationTabHelper(const OnionLocationTabHelper&) = delete;
  OnionLocationTabHelper& operator=(const OnionLocationTabHelper&) = delete;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_ONION_LOCATION_TAB_HELPER_H_
