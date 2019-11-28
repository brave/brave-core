/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_CLIENT_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_CLIENT_H_

#include "components/omnibox/browser/omnibox_client.h"

class BraveOmniboxClient : public OmniboxClient {
 public:
  virtual bool IsAutocompleteEnabled() const;

 protected:
  ~BraveOmniboxClient() override {}
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_CLIENT_H_
