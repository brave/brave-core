// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/psst_action_controller.h"

#include "base/check_deref.h"
#include "base/memory/weak_ptr.h"
#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_anchor.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/view.h"

namespace {

constexpr int kIconSize = 16;
constexpr float kBadgeRadius = 3.0f;

class PsstIconImageSource : public gfx::CanvasImageSource {
 public:
  explicit PsstIconImageSource(const gfx::IconDescription& icon)
      : CanvasImageSource(gfx::Size(icon.dip_size, icon.dip_size)),
        icon_description_(icon) {}
  ~PsstIconImageSource() override = default;

  // gfx::CanvasImageSource overrides:
  void Draw(gfx::Canvas* canvas) override {
    const gfx::ImageSkia icon_image = gfx::CreateVectorIcon(icon_description_);
    canvas->DrawImageInt(icon_image, 0, 0);

    cc::PaintFlags flags;
    flags.setColor(SK_ColorRED);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);

    const float icon_size = static_cast<float>(icon_description_.dip_size);
    canvas->DrawCircle(gfx::PointF(icon_size - kBadgeRadius, kBadgeRadius),
                       kBadgeRadius, flags);
  }

 private:
  const gfx::IconDescription icon_description_;
};

}  // namespace

namespace page_actions {

PsstActionController::PsstActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {
  CHECK(base::FeatureList::IsEnabled(psst::features::kEnablePsst));
}

PsstActionController::~PsstActionController() = default;

void PsstActionController::SetMenuModelDelegate(Delegate* delegate) {
  psst_menu_model_delegate_ = delegate;
}

void PsstActionController::SetVisible(bool visible) {
  if (visible) {
    page_action_controller_->Show(kActionShowPsstIcon);
    UpdatePageAction();
  } else {
    page_action_controller_->Hide(kActionShowPsstIcon);
  }
}

base::WeakPtr<PsstActionController> PsstActionController::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PsstActionController::ExecuteAction(
    ToolbarButtonProvider* toolbar_button_provider,
    actions::ActionItem* item,
    int event_flags) {
  if (menu_runner_ && menu_runner_->IsRunning()) {
    menu_runner_->Cancel();
  }

  // Handle left-click on the page action icon as a trigger to open the consent
  // dialog.
  if (event_flags & ui::EF_LEFT_MOUSE_BUTTON) {
    if (psst_menu_model_delegate_) {
      psst_menu_model_delegate_->OnShowConsentDialogSelected();
    }

    return;
  }

  if (!toolbar_button_provider) {
    return;
  }

  BrowserWindowInterface* const bwi = tab_->GetBrowserWindowInterface();
  if (!bwi) {
    return;
  }

  views::View* const anchor_view =
      toolbar_button_provider->GetPageActionBubbleAnchor(kActionShowPsstIcon)
          .GetIfView();
  if (!anchor_view || !anchor_view->GetWidget()) {
    return;
  }

  psst_menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  BuildMenuItems();

  menu_anchor_view_tracker_.SetView(anchor_view);

  action_item_for_menu_ = item;
  CHECK_DEREF(action_item_for_menu_).SetIsShowingBubble(true);

  menu_model_adapter_ = std::make_unique<views::MenuModelAdapter>(
      psst_menu_model_.get(),
      base::BindRepeating(&PsstActionController::OnPsstMenuClosed,
                          weak_ptr_factory_.GetWeakPtr()));
  std::unique_ptr<views::MenuItemView> root = menu_model_adapter_->CreateMenu();
  menu_runner_ = std::make_unique<views::MenuRunner>(
      std::move(root),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU);
  menu_runner_->RunMenuAt(
      anchor_view->GetWidget(), nullptr, anchor_view->GetAnchorBoundsInScreen(),
      views::MenuAnchorPosition::kTopLeft, ui::mojom::MenuSourceType::kMouse);
}

void PsstActionController::ExecuteCommand(int command_id, int event_flags) {
  if (!psst_menu_model_delegate_) {
    return;
  }
  if (command_id == IDC_PSST_DONT_SHOW_FOR_THIS_SITE) {
    psst_menu_model_delegate_->OnDontShowThisSiteSelected();
  } else if (command_id == IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING) {
    psst_menu_model_delegate_->OnDisablePrivacySettingsTuningSelected();
  }
}

bool PsstActionController::IsCommandIdEnabled(int command_id) const {
  return true;
}

void PsstActionController::BuildMenuItems() {
  psst_menu_model_->AddItem(
      IDC_PSST_DONT_SHOW_FOR_THIS_SITE,
      l10n_util::GetStringUTF16(IDS_IDC_PSST_DONT_SHOW_FOR_THIS_SITE));
  psst_menu_model_->AddItem(
      IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING,
      l10n_util::GetStringUTF16(IDS_IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING));
}

void PsstActionController::UpdatePageAction() {
  const std::u16string name =
      l10n_util::GetStringUTF16(IDS_IDC_PSST_LOCATION_BAR_BTN_TOOLTIP);
  page_action_controller_->OverrideText(kActionShowPsstIcon, name);

  page_action_controller_->OverrideAccessibleName(kActionShowPsstIcon, name);
  page_action_controller_->OverrideTooltip(kActionShowPsstIcon, name);

  page_action_controller_->SetOverrideTriggerableEvent(
      kActionShowPsstIcon,
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON);
  if (!show_badge_) {
    page_action_controller_->OverrideImage(
        kActionShowPsstIcon,
        ui::ImageModel::FromVectorIcon(kLeoPsstIcon, kColorToolbarButtonIcon,
                                       kIconSize));
    return;
  }
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    return;
  }
  const SkColor color =
      contents->GetColorProvider().GetColor(kColorToolbarButtonIcon);
  gfx::IconDescription icon_description(kLeoPsstIcon, kIconSize, color);
  gfx::ImageSkia icon_image(
      std::make_unique<PsstIconImageSource>(icon_description),
      gfx::Size(icon_description.dip_size, icon_description.dip_size));
  page_action_controller_->OverrideImage(
      kActionShowPsstIcon, ui::ImageModel::FromImageSkia(icon_image));
}

void PsstActionController::SetShowBadge(bool show) {
  if (show_badge_ == show) {
    return;
  }
  show_badge_ = show;
  UpdatePageAction();
}

void PsstActionController::OnPsstMenuClosed() {
  CHECK_DEREF(action_item_for_menu_).SetIsShowingBubble(false);
  action_item_for_menu_ = nullptr;

  if (views::View* anchor = menu_anchor_view_tracker_.view()) {
    if (views::Button* button = views::Button::AsButton(anchor)) {
      button->SetHighlighted(false);
    }
  }
  menu_anchor_view_tracker_.SetView(nullptr);

  menu_runner_.reset();
  menu_model_adapter_.reset();
  psst_menu_model_.reset();
}

}  // namespace page_actions
