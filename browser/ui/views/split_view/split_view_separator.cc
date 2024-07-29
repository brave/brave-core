/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_separator.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/split_view/split_view_menu_bubble.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {
class MenuButtonDelegate : public views::WidgetDelegateView,
                           public gfx::AnimationDelegate {
  METADATA_HEADER(MenuButtonDelegate, views::WidgetDelegateView)

 public:
  explicit MenuButtonDelegate(Browser* browser) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    constexpr auto kCornerRadius = 8;
    constexpr auto kBorderThickness = 1;
    SetBackground(views::CreateThemedRoundedRectBackground(
        kColorBraveSplitViewMenuButtonBackground, kCornerRadius,
        /*for_border_thickness*/ kBorderThickness));
    SetBorder(views::CreateThemedRoundedRectBorder(
        /*thickness*/ kBorderThickness, kCornerRadius,
        kColorBraveSplitViewMenuButtonBorder));

    auto* image_button = AddChildView(views::ImageButton::CreateIconButton(
        base::BindRepeating(&MenuButtonDelegate::OnMenuPressed,
                            base::Unretained(this), browser),
        kLeoMoreVerticalIcon,
        l10n_util::GetStringUTF16(IDS_SPLIT_VIEW_A11Y_SEPARATOR_MENU_BUTTON)));

    auto image_model = ui::ImageModel::FromVectorIcon(
        kLeoMoreVerticalIcon, kColorBraveSplitViewMenuButtonIcon,
        /*icon_size*/ 18);
    for (auto state : views::Button::kButtonStates) {
      image_button->SetImageModel(state, image_model);
    }

    image_button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
    image_button->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);

    background_animation_.SetSlideDuration(base::Milliseconds(150));

    SetNotifyEnterExitOnChild(true);
  }

  ~MenuButtonDelegate() override = default;

  // views::WidgetDelegateView:
  void OnMouseEntered(const ui::MouseEvent& event) override {
    background_animation_.Show();
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    background_animation_.Hide();
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    auto scoped_canvas = TransformCanvasForBackground(canvas);
    views::WidgetDelegateView::OnPaintBackground(canvas);
  }

  void OnPaintBorder(gfx::Canvas* canvas) override {
    auto scoped_canvas = TransformCanvasForBackground(canvas);
    views::WidgetDelegateView::OnPaintBorder(canvas);
  }

  // gfx::AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override {
    SchedulePaint();
  }

  void AnimationProgressed(const gfx::Animation* animation) override {
    SchedulePaint();
  }

 private:
  void OnMenuPressed(Browser* browser, const ui::Event& event) {
    SplitViewMenuBubble::Show(browser, this);
  }

  std::unique_ptr<gfx::ScopedCanvas> TransformCanvasForBackground(
      gfx::Canvas* canvas) {
    auto scoped_canvas = std::make_unique<gfx::ScopedCanvas>(canvas);
    gfx::Transform transform;
    transform.Translate(width() / 2, height() / 2);
    transform.Scale(gfx::Tween::DoubleValueBetween(
                        background_animation_.GetCurrentValue(), 0.4, 1),
                    1);
    transform.Translate(-width() / 2, -height() / 2);
    canvas->Transform(transform);
    return scoped_canvas;
  }

  gfx::SlideAnimation background_animation_{this};
};

BEGIN_METADATA(MenuButtonDelegate)
END_METADATA

}  // namespace

SplitViewSeparator::SplitViewSeparator(Browser* browser)
    : ResizeArea(this), browser_(browser) {}

SplitViewSeparator::~SplitViewSeparator() = default;

void SplitViewSeparator::AddedToWidget() {
  ResizeArea::AddedToWidget();

  CreateMenuButton();
}

void SplitViewSeparator::VisibilityChanged(views::View* starting_from,
                                           bool is_visible) {
  if (starting_from != this) {
    return;
  }

  if (is_visible) {
    LayoutMenuButton();
    menu_button_widget_->Show();
  } else {
    menu_button_widget_->Hide();
  }
}

bool SplitViewSeparator::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() && event.GetClickCount() == 2) {
    if (resize_area_delegate_) {
      resize_area_delegate_->OnDoubleClicked();
    }
    return true;
  }

  return ResizeArea::OnMousePressed(event);
}

void SplitViewSeparator::Layout(PassKey) {
  LayoutMenuButton();
}

void SplitViewSeparator::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  ResizeArea::ViewHierarchyChanged(details);

  if (details.is_add && details.child == this) {
    CHECK(!parent_view_observation_.IsObserving())
        << "This is supposed to be called only once.";
    parent_view_observation_.Observe(parent());
  }
}

void SplitViewSeparator::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  ResizeArea::OnBoundsChanged(previous_bounds);

  LayoutMenuButton();
}

void SplitViewSeparator::OnResize(int resize_amount, bool done_resizing) {
  // When mouse goes toward web contents area, the cursor could have been
  // changed to the normal cursor. Reset it resize cursor.
  GetWidget()->SetCursor(ui::Cursor(ui::mojom::CursorType::kEastWestResize));
  if (resize_area_delegate_) {
    resize_area_delegate_->OnResize(resize_amount, done_resizing);
  }

  if (done_resizing == menu_button_widget_->IsVisible()) {
    return;
  }

  if (done_resizing) {
    LayoutMenuButton();
    menu_button_widget_->Show();
  } else {
    menu_button_widget_->Hide();
  }
}

void SplitViewSeparator::OnWidgetBoundsChanged(views::Widget* widget,
                                               const gfx::Rect& new_bounds) {
  LayoutMenuButton();
}

void SplitViewSeparator::OnViewBoundsChanged(views::View* observed_view) {
  LayoutMenuButton();
}

void SplitViewSeparator::CreateMenuButton() {
  CHECK(!menu_button_widget_);

  menu_button_widget_ = new views::Widget();
  views::Widget::InitParams params(
      views::Widget::InitParams::Type::TYPE_CONTROL);
  params.delegate = new MenuButtonDelegate(browser_);
  params.parent = GetWidget()->GetNativeView();
  menu_button_widget_->Init(std::move(params));

  parent_widget_observation_.Observe(GetWidget());
}

void SplitViewSeparator::LayoutMenuButton() {
  if (!menu_button_widget_) {
    return;
  }

  constexpr int kMenuButtonSize = 28;
  constexpr int kMenuButtonMarginTop = 8;

  auto menu_button_bounds = ConvertRectToWidget(GetLocalBounds());
  menu_button_bounds.set_x(menu_button_bounds.top_center().x() -
                           kMenuButtonSize / 2);
  menu_button_bounds.set_y(menu_button_bounds.y() + kMenuButtonMarginTop);
  menu_button_bounds.set_size(gfx::Size(kMenuButtonSize, kMenuButtonSize));
  menu_button_widget_->SetBounds(menu_button_bounds);
}

BEGIN_METADATA(SplitViewSeparator)
END_METADATA
