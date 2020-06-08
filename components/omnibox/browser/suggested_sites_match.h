/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_MATCH_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_MATCH_H_

#include <string>

#include "base/strings/string16.h"
#include "url/gurl.h"

// This is the provider for Brave Suggested Sites
class SuggestedSitesMatch {
 public:
  SuggestedSitesMatch(const SuggestedSitesMatch& other);
  SuggestedSitesMatch(const std::string& match_string,
                      const GURL& destination_url,
                      const GURL& stripped_destination_url,
                      const base::string16& display);
  ~SuggestedSitesMatch();
  std::string match_string_;
  GURL destination_url_;
  GURL stripped_destination_url_;
  base::string16 display_;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_SUGGESTED_SITES_MATCH_H_
