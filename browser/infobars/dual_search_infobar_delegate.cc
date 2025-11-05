/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/dual_search_infobar_delegate.h"

#include <memory>

#include "base/metrics/histogram_functions.h"
#include "brave/browser/infobars/brave_confirm_infobar_creator.h"
#include "brave/browser/ui/tabs/dual_search_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"

namespace {
constexpr char kDualSearchInfoBarShownPref[] =
    "brave.omnibox.dual_search_infobar_shown";
constexpr char kDualSearchEnabledPref[] = "brave.omnibox.dual_search_enabled";
constexpr char kDualSearchInfoBarActionHistogramName[] =
    "Brave.Omnibox.DualSearchInfoBarAction";
}

// static
void DualSearchInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    PrefService* prefs,
    content::WebContents* web_contents) {
  // Only show once
  if (prefs->GetBoolean(kDualSearchInfoBarShownPref)) {
    return;
  }

  infobar_manager->AddInfoBar(
      CreateBraveConfirmInfoBar(std::unique_ptr<BraveConfirmInfoBarDelegate>(
          new DualSearchInfoBarDelegate(prefs, web_contents))));
}

DualSearchInfoBarDelegate::DualSearchInfoBarDelegate(PrefService* prefs,
                                                     content::WebContents* web_contents)
    : prefs_(prefs), web_contents_(web_contents) {}

DualSearchInfoBarDelegate::~DualSearchInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
DualSearchInfoBarDelegate::GetIdentifier() const {
  return DUAL_SEARCH_INFOBAR_DELEGATE;
}

ui::ImageModel DualSearchInfoBarDelegate::GetIcon() const {
  return ui::ImageModel::FromResourceId(IDR_BRAVE_DUAL_SEARCH_SPLIT_VIEW_IMG);
}

std::u16string DualSearchInfoBarDelegate::GetMessageText() const {
  return u"Split your search: Google on one side, Brave on the other.";
}

int DualSearchInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL | BUTTON_EXTRA;
}

std::u16string DualSearchInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_OK) {
    return u"Keep split view";
  } else if (button == BUTTON_CANCEL) {
    return u"Keep Google";
  } else if (button == BUTTON_EXTRA) {
    return u"Switch to Brave";
  }
  return u"";
}

bool DualSearchInfoBarDelegate::Accept() {
  // Keep dual search enabled - mark as shown
  prefs_->SetBoolean(kDualSearchInfoBarShownPref, true);
  // Record P3A: 0 = Keep split view
  base::UmaHistogramExactLinear(kDualSearchInfoBarActionHistogramName, 0, 4);
  return true;
}

bool DualSearchInfoBarDelegate::Cancel() {
  // "Keep Google" - Disable dual search and close Brave Search tab
  prefs_->SetBoolean(kDualSearchEnabledPref, false);
  prefs_->SetBoolean(kDualSearchInfoBarShownPref, true);

  // Record P3A: 1 = Keep Google
  base::UmaHistogramExactLinear(kDualSearchInfoBarActionHistogramName, 1, 4);

  // Close the Brave Search tab (paired tab)
  CloseOtherTab(false);

  return true;
}

bool DualSearchInfoBarDelegate::ExtraButtonPressed() {
  // "Switch to Brave" - Disable dual search, close Google tab, switch default search
  prefs_->SetBoolean(kDualSearchEnabledPref, false);
  prefs_->SetBoolean(kDualSearchInfoBarShownPref, true);

  // Record P3A: 2 = Switch to Brave
  base::UmaHistogramExactLinear(kDualSearchInfoBarActionHistogramName, 2, 4);

  // Change default search engine to Brave Search
  Browser* browser = chrome::FindBrowserWithTab(web_contents_);
  if (browser) {
    auto* template_url_service =
        TemplateURLServiceFactory::GetForProfile(browser->profile());
    if (template_url_service) {
      auto template_urls = template_url_service->GetTemplateURLs();
      for (const auto& turl : template_urls) {
        if (turl->keyword() == u"search.brave.com" ||
            turl->url().find("search.brave.com") != std::string::npos) {
          template_url_service->SetUserSelectedDefaultSearchProvider(turl);
          break;
        }
      }
    }
  }

  // Close the Google/default search tab (paired tab)
  CloseOtherTab(true);

  return true;
}

void DualSearchInfoBarDelegate::InfoBarDismissed() {
  // User closed the infobar with X - mark as shown
  prefs_->SetBoolean(kDualSearchInfoBarShownPref, true);
  // Record P3A: 3 = Dismissed without action
  base::UmaHistogramExactLinear(kDualSearchInfoBarActionHistogramName, 3, 4);
}

bool DualSearchInfoBarDelegate::ShouldSupportMultiLine() const {
  return false;
}

std::vector<int> DualSearchInfoBarDelegate::GetButtonsOrder() const {
  // Order: Keep split view, Keep Google, Switch to Brave
  return {BUTTON_OK, BUTTON_CANCEL, BUTTON_EXTRA};
}

void DualSearchInfoBarDelegate::CloseOtherTab(bool close_default_search) {
  auto* helper = DualSearchTabHelper::FromWebContents(web_contents_);
  if (!helper || !helper->paired_tab()) {
    return;
  }

  auto* paired_tab = helper->paired_tab();
  Browser* browser = chrome::FindBrowserWithTab(web_contents_);
  if (!browser) {
    return;
  }

  TabStripModel* tab_strip = browser->tab_strip_model();

  // Determine which tab to close based on URL
  content::WebContents* tab_to_close = nullptr;
  std::string this_url = web_contents_->GetLastCommittedURL().host();
  bool this_is_brave = this_url.find("search.brave.com") != std::string::npos;

  if (close_default_search) {
    // Close the Google/default search tab
    tab_to_close = this_is_brave ? paired_tab : web_contents_.get();
  } else {
    // Close the Brave Search tab
    tab_to_close = this_is_brave ? web_contents_.get() : paired_tab;
  }

  int close_index = tab_strip->GetIndexOfWebContents(tab_to_close);
  if (close_index != TabStripModel::kNoTab) {
    // Clear paired tab references
    auto* close_helper = DualSearchTabHelper::FromWebContents(tab_to_close);
    if (close_helper) {
      close_helper->SetPairedTab(nullptr);
    }

    content::WebContents* keep_tab = (tab_to_close == paired_tab)
                                        ? web_contents_.get()
                                        : paired_tab;
    auto* keep_helper = DualSearchTabHelper::FromWebContents(keep_tab);
    if (keep_helper) {
      keep_helper->SetPairedTab(nullptr);
    }

    tab_strip->CloseWebContentsAt(close_index, TabCloseTypes::CLOSE_USER_GESTURE);
  }
}
