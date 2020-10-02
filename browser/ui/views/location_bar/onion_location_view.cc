/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/onion_location_view.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/tor/onion_location_tab_helper.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"

OnionLocationView::OnionLocationView()
    : LabelButton(this, base::ASCIIToUTF16("Open in Tor")) {
  // label()->SetBackgroundColor(SkColorSetRGB(98,60,129));
  SetVisible(false);
}

OnionLocationView::~OnionLocationView() {}

void OnionLocationView::Update(content::WebContents* web_contents) {
  if (!web_contents)
    return;
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  if (!helper)
    return;
  SetVisible(helper->should_show_icon());
  onion_location_ = helper->onion_location();
}

void OnionLocationView::ButtonPressed(Button* sender, const ui::Event& event) {
  profiles::SwitchToTorProfile(base::BindRepeating(
      &OnionLocationView::OnTorProfileCreated, weak_ptr_factory_.GetWeakPtr(),
      GURL(onion_location_)));
}

void OnionLocationView::OnTorProfileCreated(
    GURL onion_location,
    Profile* profile,
    Profile::CreateStatus status) {
  if (status != Profile::CreateStatus::CREATE_STATUS_INITIALIZED)
    return;
  Browser* browser = chrome::FindTabbedBrowser(profile, true);
  if (!browser)
    return;
  content::OpenURLParams open_tor(onion_location, content::Referrer(),
                                  WindowOpenDisposition::OFF_THE_RECORD,
                                  ui::PAGE_TRANSITION_TYPED, false);
  browser->OpenURL(open_tor);
}
