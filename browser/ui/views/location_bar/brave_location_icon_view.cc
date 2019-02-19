/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_location_icon_view.h"

#include "chrome/grit/chromium_strings.h"
#include "content/public/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"

bool BraveLocationIconView::ShouldShowText() const {
  const auto* location_bar_model = delegate_->GetLocationBarModel();
  if (!location_bar_model->input_in_progress()) {
    const GURL& url = location_bar_model->GetURL();
    if (url.SchemeIs(content::kBraveUIScheme))
      return true;
  }

  return LocationIconView::ShouldShowText();
}

base::string16 BraveLocationIconView::GetText() const {
  if (delegate_->GetLocationBarModel()->GetURL().SchemeIs(
          content::kBraveUIScheme)) {
    return l10n_util::GetStringUTF16(IDS_SHORT_PRODUCT_NAME);
  }

  return LocationIconView::GetText();
}
