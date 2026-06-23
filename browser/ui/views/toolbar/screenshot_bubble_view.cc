// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/screenshot_bubble_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/screenshot/screenshot_controller.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "printing/buildflags/buildflags.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/dialog_model.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_model_host.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/widget/widget.h"

namespace screenshot {

namespace {

constexpr int kBubbleWidth = 280;
constexpr int kRowSpacing = 8;
constexpr int kIconSize = 16;

}  // namespace

std::unique_ptr<views::BubbleDialogModelHost> ShowScreenshotBubble(
    content::WebContents* web_contents,
    views::View* anchor,
    ScreenshotController* controller) {
  CHECK(controller) << "ScreenshotController must be non-null";
  const bool can_capture = controller->CanCapture(web_contents).has_value();

  auto container = std::make_unique<views::BoxLayoutView>();
  container->SetOrientation(views::BoxLayout::Orientation::kVertical);
  container->SetBetweenChildSpacing(kRowSpacing);

  auto* visible_btn =
      container->AddChildView(std::make_unique<views::MdTextButton>(
          views::Button::PressedCallback(),
          l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_VISIBLE_AREA)));
  visible_btn->SetImageModel(
      views::Button::ButtonState::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoScreenshotIcon,
                                     ui::kColorButtonForeground, kIconSize));
  visible_btn->SetEnabled(can_capture);

  // Set callbacks now that raw pointers are available. Each callback closes the
  // bubble (via the button's own Widget) before starting capture so the UI is
  // cleared first.
  visible_btn->SetCallback(base::BindRepeating(
      [](content::WebContents* wc, ScreenshotController* c,
         views::Button* btn) {
        btn->SetCallback(views::Button::PressedCallback());
        btn->GetWidget()->CloseWithReason(
            views::Widget::ClosedReason::kUnspecified);
        c->CaptureVisibleArea(wc, base::DoNothing());
      },
      base::Unretained(web_contents), base::Unretained(controller),
      base::Unretained(visible_btn)));

  auto* selected_btn =
      container->AddChildView(std::make_unique<views::MdTextButton>(
          views::Button::PressedCallback(),
          l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_SELECTED_AREA)));
  selected_btn->SetImageModel(
      views::Button::ButtonState::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoScreenshotIcon,
                                     ui::kColorButtonForeground, kIconSize));
  selected_btn->SetEnabled(can_capture);

  selected_btn->SetCallback(base::BindRepeating(
      [](content::WebContents* wc, ScreenshotController* c,
         views::Button* btn) {
        btn->SetCallback(views::Button::PressedCallback());
        btn->GetWidget()->CloseWithReason(
            views::Widget::ClosedReason::kUnspecified);
        c->CaptureSelectedArea(wc, base::DoNothing());
      },
      base::Unretained(web_contents), base::Unretained(controller),
      base::Unretained(selected_btn)));

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  auto* full_btn =
      container->AddChildView(std::make_unique<views::MdTextButton>(
          views::Button::PressedCallback(),
          l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_FULL_PAGE)));
  full_btn->SetImageModel(
      views::Button::ButtonState::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoFullscreenOnIcon,
                                     ui::kColorButtonForeground, kIconSize));
  full_btn->SetEnabled(can_capture);

  full_btn->SetCallback(base::BindRepeating(
      [](content::WebContents* wc, ScreenshotController* c,
         views::Button* btn) {
        btn->SetCallback(views::Button::PressedCallback());
        btn->GetWidget()->CloseWithReason(
            views::Widget::ClosedReason::kUnspecified);
        c->CaptureFullPage(wc, base::DoNothing());
      },
      base::Unretained(web_contents), base::Unretained(controller),
      base::Unretained(full_btn)));
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

  auto model =
      ui::DialogModel::Builder()
          .SetTitle(
              l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_BUBBLE_TITLE))
          .OverrideShowCloseButton(true)
          .AddCustomField(
              std::make_unique<views::BubbleDialogModelHost::CustomView>(
                  std::move(container),
                  views::BubbleDialogModelHost::FieldType::kControl))
          .Build();

  auto bubble = std::make_unique<views::BubbleDialogModelHost>(
      std::move(model), anchor, views::BubbleBorder::TOP_RIGHT);
  bubble->set_fixed_width(kBubbleWidth);
  bubble->set_margins(gfx::Insets::TLBR(0, 16, 16, 16));

  return bubble;
}

}  // namespace screenshot
