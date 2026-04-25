/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_CLICKED_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_CLICKED_INFOBAR_DELEGATE_H_

#include <string>
#include <vector>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "components/infobars/core/infobar_delegate.h"

class PrefService;
class GURL;
enum class WindowOpenDisposition;

namespace content {
class WebContents;
}  // namespace content

namespace ui {
class ImageModel;
}  // namespace ui

namespace brave_ads {

class CreativeSearchResultAdClickedInfoBarDelegate
    : public BraveConfirmInfoBarDelegate {
 public:
  CreativeSearchResultAdClickedInfoBarDelegate();

  CreativeSearchResultAdClickedInfoBarDelegate(
      const CreativeSearchResultAdClickedInfoBarDelegate&) = delete;
  CreativeSearchResultAdClickedInfoBarDelegate& operator=(
      const CreativeSearchResultAdClickedInfoBarDelegate&) = delete;

  ~CreativeSearchResultAdClickedInfoBarDelegate() override;

  static void Create(content::WebContents* web_contents, PrefService* prefs);

 private:
  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  ui::ImageModel GetIcon() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

  // BraveConfirmInfoBarDelegate:
  std::vector<int> GetButtonsOrder() const override;
  bool ShouldSupportMultiLine() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_CLICKED_INFOBAR_DELEGATE_H_
