/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/suggested_sites_match.h"

// This is the provider for Brave Suggested Sites
SuggestedSitesMatch::SuggestedSitesMatch(const std::string& match_string,
                                         const GURL& destination_url,
                                         const GURL& stripped_destination_url,
                                         const std::u16string& display)
    : match_string_(match_string),
      destination_url_(destination_url),
      stripped_destination_url_(stripped_destination_url),
      display_(display) {}

SuggestedSitesMatch::SuggestedSitesMatch(const SuggestedSitesMatch& other) {
  match_string_ = other.match_string_;
  destination_url_ = other.destination_url_;
  stripped_destination_url_ = other.stripped_destination_url_;
  display_ = other.display_;
}

SuggestedSitesMatch::~SuggestedSitesMatch() {
}
