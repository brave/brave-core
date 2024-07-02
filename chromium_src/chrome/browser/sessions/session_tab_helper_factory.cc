/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#include "components/sessions/content/session_tab_helper.h"

namespace brave {

class BraveSessionTabHelper : public sessions::SessionTabHelper {
 public:
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override {
    if (request_otr::RequestOTRStorageTabHelper* tab_storage =
            request_otr::RequestOTRStorageTabHelper::FromWebContents(
                web_contents())) {
      if (tab_storage->has_offered_otr()) {
        return;
      }
    }
    sessions::SessionTabHelper::NavigationEntryCommitted(load_details);
  }

  static void Create(content::WebContents* contents, DelegateLookup lookup) {
    contents->SetUserData(&sessions::SessionTabHelper::kUserDataKey,
                          base::WrapUnique(new BraveSessionTabHelper(
                              contents, std::move(lookup))));
  }

 private:
  BraveSessionTabHelper(content::WebContents* contents, DelegateLookup lookup)
      : SessionTabHelper(contents, lookup) {}
};

}  // namespace brave

// This is a way to redefine CreateForWebContents to call our custom create
// method (above) without actually patching the upstream file. The upstream
// code checks FromWebContents and exits before this is called, so this ternary
// will only ever execute if the tab helper does not already exist.
#define CreateForWebContents(CONTENTS, LOOKUP)                 \
  FromWebContents(CONTENTS)                                    \
      ? brave::BraveSessionTabHelper::Create(CONTENTS, LOOKUP) \
      : brave::BraveSessionTabHelper::Create(CONTENTS, LOOKUP)

#include "src/chrome/browser/sessions/session_tab_helper_factory.cc"

#undef CreateForWebContents
