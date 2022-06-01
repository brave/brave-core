/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"

#include "brave/browser/ui/views/translate/brave_translate_icon_view.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "components/prefs/pref_service.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "extensions/browser/extension_registry.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/table_layout.h"
#include "ui/views/style/platform_style.h"

BraveTranslateBubbleView::BraveTranslateBubbleView(
    views::View* anchor_view,
    std::unique_ptr<TranslateBubbleModel> model,
    translate::TranslateErrors::Type error_type,
    content::WebContents* web_contents,
    base::OnceClosure on_closing)
    : TranslateBubbleView(anchor_view,
                          std::move(model),
                          error_type,
                          web_contents,
                          std::move(on_closing)) {}

BraveTranslateBubbleView::~BraveTranslateBubbleView() {
}

std::unique_ptr<views::View>
BraveTranslateBubbleView::BraveCreateViewBeforeTranslate() {
  const float kPaddingResizesEqually = 1.0;
  auto view = std::make_unique<views::View>();
  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  view->SetLayoutManager(std::make_unique<views::TableLayout>())
      ->AddColumn(views::LayoutAlignment::kStart,
                  views::LayoutAlignment::kCenter,
                  views::TableLayout::kFixedSize,
                  views::TableLayout::ColumnSize::kUsePreferred, 0, 0)
      .AddPaddingColumn(kPaddingResizesEqually, 0)
      .AddColumn(views::LayoutAlignment::kStart,
                 views::LayoutAlignment::kCenter,
                 views::TableLayout::kFixedSize,
                 views::TableLayout::ColumnSize::kUsePreferred, 0, 0)
      .AddPaddingColumn(views::TableLayout::kFixedSize,
                        provider->GetDistanceMetric(
                            views::DISTANCE_RELATED_BUTTON_HORIZONTAL))
      .AddColumn(views::LayoutAlignment::kStart,
                 views::LayoutAlignment::kCenter,
                 views::TableLayout::kFixedSize,
                 views::TableLayout::ColumnSize::kUsePreferred, 0, 0)
      .AddPaddingRow(views::TableLayout::kFixedSize,
                     provider->GetDistanceMetric(
                         views::DISTANCE_UNRELATED_CONTROL_VERTICAL))
      .AddRows(1, views::TableLayout::kFixedSize);

  auto dont_ask_button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&BraveTranslateBubbleView::ButtonPressed,
                          base::Unretained(this), BUTTON_ID_ALWAYS_TRANSLATE),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_TRANSLATE_BUBBLE_DONT_ASK_AGAIN));
  dont_ask_button->SetID(BUTTON_ID_ALWAYS_TRANSLATE);

  auto accept_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&BraveTranslateBubbleView::ButtonPressed,
                          base::Unretained(this), BUTTON_ID_DONE),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_TRANSLATE_BUBBLE_INSTALL));
  accept_button->SetID(BUTTON_ID_DONE);
  accept_button->SetIsDefault(true);

  auto cancel_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&BraveTranslateBubbleView::ButtonPressed,
                          base::Unretained(this), BUTTON_ID_CLOSE),
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_TRANSLATE_BUBBLE_CANCEL));
  cancel_button->SetID(BUTTON_ID_CLOSE);

  // We can't call views::style::GetColor() until the associated widget has been
  // initialized or views::View::GetNativeTheme() will be called to avoid using
  // the global NativeInstance, which would be an error.
  RegisterWidgetInitializedCallback(base::BindOnce(
      [](views::LabelButton* dont_ask_button) {
        const auto color = views::style::GetColor(
            *dont_ask_button, views::style::CONTEXT_BUTTON_MD,
            views::style::STYLE_PRIMARY);
        dont_ask_button->SetTextColor(views::Button::STATE_NORMAL, color);
      },
      base::Unretained(dont_ask_button.get())));

  view->AddChildView(std::move(dont_ask_button));

  if (views::PlatformStyle::kIsOkButtonLeading) {
    view->AddChildView(std::move(accept_button));
    view->AddChildView(std::move(cancel_button));
  } else {
    view->AddChildView(std::move(cancel_button));
    view->AddChildView(std::move(accept_button));
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
        ->GetPageActionIconView(PageActionIconType::kTranslate));
  DCHECK(translate_icon);

  translate_icon->InstallGoogleTranslate();
}

void BraveTranslateBubbleView::DisableOfferTranslatePref() {
  if (!web_contents())
    return;

  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  PrefService* const prefs = profile->GetOriginalProfile()->GetPrefs();
  DCHECK(prefs);

  prefs->SetBoolean(translate::prefs::kOfferTranslateEnabled, false);
}

void BraveTranslateBubbleView::ButtonPressed(ButtonID button_id) {
  switch (button_id) {
    case BUTTON_ID_DONE: {
      InstallGoogleTranslate();
      break;
    }
    case BUTTON_ID_CLOSE: {
      CloseBubble();
      break;
    }
    case BUTTON_ID_ALWAYS_TRANSLATE: {
      DisableOfferTranslatePref();
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

bool BraveTranslateBubbleView::ShouldShowWindowTitle() const {
  return true;
}

void BraveTranslateBubbleView::Init() {
  TranslateBubbleView::Init();
  removed_translate_view_ = RemoveChildViewT(translate_view_.get());
  translate_view_ = AddChildView(BraveCreateViewBeforeTranslate());
}

int BraveTranslateBubbleView::GetTitleBeforeTranslateTitle() {
  return IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_INSTALL_TITLE;
}
