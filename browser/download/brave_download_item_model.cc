/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_item_model.h"

#include <string>

#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/common/origin_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/text_constants.h"
#include "ui/gfx/text_elider.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using download::DownloadItem;

BraveDownloadItemModel::BraveDownloadItemModel(DownloadUIModel& model)
    : model_(model) {}

BraveDownloadItemModel::~BraveDownloadItemModel() {}

// Adds origin url to the tooltip text and "Not secure", if needed.
base::string16 BraveDownloadItemModel::GetTooltipText(
    const gfx::FontList& font_list,
    int max_width) {
  base::string16 tooltip = model_.GetTooltipText(font_list, max_width);

  bool is_secure;
  base::string16 origin_url = GetOriginURLText(is_secure);

  if (!origin_url.empty()) {
    tooltip += base::ASCIIToUTF16("\n");
    base::string16 tooltip_extra;
    if (!is_secure)
      tooltip_extra = l10n_util::GetStringUTF16(IDS_NOT_SECURE_VERBOSE_STATE) +
                      base::char16(' ');
    tooltip_extra += origin_url;
    tooltip +=
        gfx::ElideText(tooltip_extra, font_list, max_width, gfx::ELIDE_TAIL);
  }

  return tooltip;
}

// Returns origin url text and sets |is_secure|.
base::string16 BraveDownloadItemModel::GetOriginURLText(bool& is_secure) {
  is_secure = false;
  const GURL gurl = model_.download()->GetURL();
  if (gurl.is_empty()) {
    return base::string16();
  }

  std::string origin;
  if (gurl.is_valid()) {
    is_secure = content::IsOriginSecure(gurl);
    if (gurl.SchemeIs(url::kAboutScheme)) {
      origin = gurl.spec();
    } else {
      origin = gurl.GetOrigin().spec();
      if (!gurl.SchemeIsFile()) {
        base::TrimString(origin, "/", &origin);
      }
    }
  } else {
    origin = gurl.possibly_invalid_spec();
  }

  return base::UTF8ToUTF16(origin);
}
