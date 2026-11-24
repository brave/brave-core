/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/public/peek_controller.h"

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

namespace {

class PeekContentsData : public content::WebContentsUserData<PeekContentsData> {
 public:
  ~PeekContentsData() override = default;

  PeekController& controller() { return *controller_; }

 private:
  friend class content::WebContentsUserData<PeekContentsData>;
  PeekContentsData(content::WebContents* web_contents,
                   PeekController& controller)
      : content::WebContentsUserData<PeekContentsData>(*web_contents),
        controller_(controller) {}

  const raw_ref<PeekController> controller_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(PeekContentsData);

}  // namespace

DEFINE_USER_DATA(PeekController);

PeekController::PeekController(BrowserWindowInterface& browser)
    : scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {}

PeekController::~PeekController() = default;

// static
PeekController* PeekController::From(BrowserWindowInterface* browser) {
  return browser ? Get(browser->GetUnownedUserDataHost()) : nullptr;
}

// static
PeekController* PeekController::FromPeekContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }
  auto* data = PeekContentsData::FromWebContents(web_contents);
  return data ? &data->controller() : nullptr;
}

void PeekController::SetShowingContents(content::WebContents* web_contents,
                                        bool showing) {
  if (showing) {
    PeekContentsData::CreateForWebContents(web_contents, *this);
  } else {
    web_contents->RemoveUserData(PeekContentsData::UserDataKey());
  }
}
