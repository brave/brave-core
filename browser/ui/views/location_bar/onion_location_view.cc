/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/onion_location_view.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/onion_location_tab_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"

namespace {

constexpr SkColor kOpenInTorBg = SkColorSetRGB(0x6a, 0x37, 0x85);

void OnTorProfileCreated(GURL onion_location,
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

}  // namespace

OnionLocationView::OnionLocationView(Profile* profile)
    : LabelButton(this,
                  l10n_util::GetStringUTF16(IDS_LOCATION_BAR_OPEN_IN_TOR)) {
  if (brave::IsTorProfile(profile))
    SetText(l10n_util::GetStringUTF16(IDS_LOCATION_BAR_ONION_AVAILABLE));
  SetBackground(views::CreateSolidBackground(kOpenInTorBg));
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia image;
  const SkBitmap bitmap =
      rb.GetImageNamed(IDR_BRAVE_OPEN_IN_TOR_WINDOW_IMG).AsBitmap();
  image.AddRepresentation(gfx::ImageSkiaRep(bitmap, 1));
  SetImage(views::Button::STATE_NORMAL, image);
  SetImageLabelSpacing(6);
  SetVisible(false);

  SetInkDropMode(InkDropMode::ON);
  set_has_ink_drop_action_on_click(true);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  set_ink_drop_visible_opacity(kToolbarInkDropVisibleOpacity);
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
  profiles::SwitchToTorProfile(
      base::BindRepeating(&OnTorProfileCreated, GURL(onion_location_)));
}
