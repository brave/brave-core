/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"

#include "brave/browser/ui/views/translate/brave_translate_icon_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/omnibox_page_action_icon_container_view.h"
#include "chrome/browser/ui/page_action/page_action_icon_container.h"
#include "extensions/browser/extension_registry.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/style/platform_style.h"

BraveTranslateBubbleView::BraveTranslateBubbleView(
    views::View* anchor_view,
    std::unique_ptr<TranslateBubbleModel> model,
    translate::TranslateErrors::Type error_type,
    content::WebContents* web_contents)
    : TranslateBubbleView(anchor_view,
                          std::move(model),
                          error_type,
                          web_contents) {
}

BraveTranslateBubbleView::~BraveTranslateBubbleView() {
}

views::View* BraveTranslateBubbleView::BraveCreateViewBeforeTranslate() {
  views::View* view = new views::View();
  views::GridLayout* layout =
      view->SetLayoutManager(std::make_unique<views::GridLayout>());
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  constexpr int kButtonColumnSetId = 0;
  views::ColumnSet* cs = layout->AddColumnSet(kButtonColumnSetId);
  cs->AddPaddingColumn(1.0, 0);
  cs->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize, views::GridLayout::USE_PREF, 0,
                0);
  cs->AddPaddingColumn(
      views::GridLayout::kFixedSize,
      provider->GetDistanceMetric(views::DISTANCE_RELATED_BUTTON_HORIZONTAL));
  cs->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize, views::GridLayout::USE_PREF, 0,
                0);

  auto accept_button = views::MdTextButton::CreateSecondaryUiButton(
      this, l10n_util::GetStringUTF16(IDS_BRAVE_TRANSLATE_BUBBLE_INSTALL));
  accept_button->SetID(BUTTON_ID_TRANSLATE);
  accept_button->SetIsDefault(true);

  auto cancel_button = views::MdTextButton::CreateSecondaryUiButton(
      this, l10n_util::GetStringUTF16(IDS_BRAVE_TRANSLATE_BUBBLE_CANCEL));
  cancel_button->SetID(BUTTON_ID_CANCEL);

  layout->StartRowWithPadding(
      views::GridLayout::kFixedSize, kButtonColumnSetId,
      views::GridLayout::kFixedSize,
      provider->GetDistanceMetric(views::DISTANCE_UNRELATED_CONTROL_VERTICAL));

  if (views::PlatformStyle::kIsOkButtonLeading) {
    layout->AddView(std::move(accept_button));
    layout->AddView(std::move(cancel_button));
  } else {
    layout->AddView(std::move(cancel_button));
    layout->AddView(std::move(accept_button));
  }

  return view;
}

void BraveTranslateBubbleView::InstallGoogleTranslate() {
  if (!web_contents())
    return;
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  if (!browser)
    return;

  BraveTranslateIconView* translate_icon =
    static_cast<BraveTranslateIconView*>(
        BrowserView::GetBrowserViewForBrowser(browser)
        ->toolbar_button_provider()
        ->GetOmniboxPageActionIconContainerView()
        ->GetPageActionIconView(PageActionIconType::kTranslate));
  DCHECK(translate_icon);

  translate_icon->InstallGoogleTranslate();
}

void BraveTranslateBubbleView::ButtonPressed(views::Button* sender,
                                             const ui::Event& event) {
  switch (static_cast<ButtonID>(sender->GetID())) {
    case BUTTON_ID_TRANSLATE: {
      InstallGoogleTranslate();
      break;
    }
    case BUTTON_ID_CANCEL: {
      CloseBubble();
      break;
    }
    default: {
      // We don't expect other buttons used by chromium's original views.
      NOTREACHED();
    }
  }
}

bool BraveTranslateBubbleView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  switch (model_->GetViewState()) {
    case TranslateBubbleModel::VIEW_STATE_BEFORE_TRANSLATE: {
      if (accelerator.key_code() == ui::VKEY_RETURN) {
        InstallGoogleTranslate();
        return true;
      }
      break;
    }
    default: {
      // We don't expect views in other states.
      NOTREACHED();
    }
  }

  return TranslateBubbleView::AcceleratorPressed(accelerator);
}

void BraveTranslateBubbleView::Init() {
  TranslateBubbleView::Init();
  RemoveChildView(before_translate_view_);
  before_translate_view_ = AddChildView(BraveCreateViewBeforeTranslate());
}
