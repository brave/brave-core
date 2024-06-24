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
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/split_view/split_view_menu_bubble.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/compositor/layer.h"
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

constexpr int kMenuButtonSize = 28;

class MenuButtonDelegate : public views::WidgetDelegateView,
                           public gfx::AnimationDelegate {
  METADATA_HEADER(MenuButtonDelegate, views::WidgetDelegateView)

 public:
  MenuButtonDelegate(Browser* browser, SplitViewSeparator* separator)
      : separator_(separator) {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    constexpr auto kCornerRadius = 8;
    constexpr auto kBorderThickness = 1;
    SetBackground(views::CreateThemedRoundedRectBackground(
        kColorBraveSplitViewMenuButtonBackground, kCornerRadius,
        /*for_border_thickness*/ kBorderThickness));
    SetBorder(views::CreateThemedRoundedRectBorder(
        /*thickness*/ kBorderThickness, kCornerRadius,
        kColorBraveSplitViewMenuButtonBorder));

    image_button_ = AddChildView(views::ImageButton::CreateIconButton(
        base::BindRepeating(&MenuButtonDelegate::OnMenuPressed,
                            base::Unretained(this), browser),
        kLeoMoreVerticalIcon,
        l10n_util::GetStringUTF16(IDS_SPLIT_VIEW_A11Y_SEPARATOR_MENU_BUTTON)));

    auto image_model = ui::ImageModel::FromVectorIcon(
        kLeoMoreVerticalIcon, kColorBraveSplitViewMenuButtonIcon,
        /*icon_size*/ 18);
    for (auto state : views::Button::kButtonStates) {
      image_button_->SetImageModel(state, image_model);
    }

    image_button_->SetImageHorizontalAlignment(
        views::ImageButton::ALIGN_CENTER);
    image_button_->SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);

    UpdateImage();

    background_animation_.SetSlideDuration(base::Milliseconds(150));

    SetNotifyEnterExitOnChild(true);
  }

  ~MenuButtonDelegate() override = default;

  bool should_rotate() const {
    return separator_->orientation() ==
           SplitViewBrowserData::Orientation::kHorizontal;
  }

  void UpdateImage() {
    if (should_rotate()) {
      if (!image_button_->layer()) {
        image_button_->SetPaintToLayer();
        image_button_->layer()->SetFillsBoundsOpaquely(false);

        gfx::Transform transform;
        transform.Translate(image_button_->width() / 2,
                            image_button_->height() / 2);
        transform.Rotate(90);
        transform.Translate(-image_button_->width() / 2,
                            -image_button_->height() / 2);

        image_button_->layer()->SetTransform(transform);
      }
    } else {
      if (image_button_->layer()) {
        image_button_->DestroyLayer();
      }
    }
    SchedulePaint();
  }

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
    transform.Translate(kMenuButtonSize / 2, kMenuButtonSize / 2);
    if (should_rotate()) {
      transform.Scale(1, gfx::Tween::DoubleValueBetween(
                             background_animation_.GetCurrentValue(), 0.4, 1));
      transform.Rotate(90);
    } else {
      transform.Scale(gfx::Tween::DoubleValueBetween(
                          background_animation_.GetCurrentValue(), 0.4, 1),
                      1);
    }
    transform.Translate(-kMenuButtonSize / 2, -kMenuButtonSize / 2);
    canvas->Transform(transform);
    return scoped_canvas;
  }

  gfx::SlideAnimation background_animation_{this};

  raw_ptr<SplitViewSeparator> separator_ = nullptr;
  raw_ptr<views::ImageButton> image_button_ = nullptr;
};

BEGIN_METADATA(MenuButtonDelegate)
END_METADATA

}  // namespace

SplitViewSeparator::SplitViewSeparator(Browser* browser)
    : ResizeArea(this), browser_(browser) {}

SplitViewSeparator::~SplitViewSeparator() = default;

void SplitViewSeparator::SetOrientation(
    SplitViewBrowserData::Orientation orientation) {
  if (orientation_ == orientation) {
    return;
  }

  orientation_ = orientation;
  if (menu_button_widget_) {
    static_cast<MenuButtonDelegate*>(menu_button_widget_->widget_delegate())
        ->UpdateImage();
  }
}
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

void SplitViewSeparator::OnGestureEvent(ui::GestureEvent* event) {
  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    ResizeArea::OnGestureEvent(event);
    return;
  }

  if (event->type() == ui::ET_GESTURE_TAP_DOWN) {
    SetInitialPosition(*event);
    event->SetHandled();
  } else if (event->type() == ui::ET_GESTURE_SCROLL_BEGIN ||
             event->type() == ui::ET_GESTURE_SCROLL_UPDATE) {
    OnResize(ConvertYToScreen(*event) - initial_y_position_in_screen_,
             /*done_resizing*/ false);
    event->SetHandled();
  } else if (event->type() == ui::ET_GESTURE_END) {
    OnResize(ConvertYToScreen(*event) - initial_y_position_in_screen_,
             /*done_resizing*/ true);
    event->SetHandled();
  }
}

bool SplitViewSeparator::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() && event.GetClickCount() == 2) {
    if (resize_area_delegate_) {
      resize_area_delegate_->OnDoubleClicked();
    }

    LayoutMenuButton();
    return true;
  }

  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    return ResizeArea::OnMousePressed(event);
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return false;
  }

  SetInitialPosition(event);
  return true;
}

bool SplitViewSeparator::OnMouseDragged(const ui::MouseEvent& event) {
  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    return ResizeArea::OnMouseDragged(event);
  }

  if (!event.IsLeftMouseButton()) {
    return false;
  }

  OnResize(ConvertYToScreen(event) - initial_y_position_in_screen_,
           /*done_resizing*/ false);
  return true;
}

void SplitViewSeparator::OnMouseReleased(const ui::MouseEvent& event) {
  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    ResizeArea::OnMouseReleased(event);
    return;
  }

  OnResize(ConvertYToScreen(event) - initial_y_position_in_screen_,
           /*done_resizing*/ true);
}

void SplitViewSeparator::OnMouseCaptureLost() {
  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    ResizeArea::OnMouseCaptureLost();
    return;
  }

  OnResize(initial_y_position_in_screen_, /*done_resizing*/ true);
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

ui::Cursor SplitViewSeparator::GetCursor(const ui::MouseEvent& event) {
  return GetCursor();
}

ui::Cursor SplitViewSeparator::GetCursor() {
  return ui::Cursor(orientation_ == SplitViewBrowserData::Orientation::kVertical
                        ? ui::mojom::CursorType::kEastWestResize
                        : ui::mojom::CursorType::kNorthSouthResize);
}

void SplitViewSeparator::OnResize(int resize_amount, bool done_resizing) {
  // When mouse goes toward web contents area, the cursor could have been
  // changed to the normal cursor. Reset it resize cursor.
  GetWidget()->SetCursor(GetCursor());
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
  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::Type::TYPE_CONTROL;
  params.delegate = new MenuButtonDelegate(browser_, this);
  params.parent = GetWidget()->GetNativeView();
  menu_button_widget_->Init(std::move(params));

  parent_widget_observation_.Observe(GetWidget());
}

void SplitViewSeparator::LayoutMenuButton() {
  if (!menu_button_widget_) {
    return;
  }

  constexpr int kMenuButtonMarginTop = 8;

  auto menu_button_bounds = ConvertRectToWidget(GetLocalBounds());
  menu_button_bounds.set_x(menu_button_bounds.top_center().x() -
                           kMenuButtonSize / 2);
  if (orientation_ == SplitViewBrowserData::Orientation::kVertical) {
    menu_button_bounds.set_y(menu_button_bounds.y() + kMenuButtonMarginTop);
  } else {
    menu_button_bounds.set_y(menu_button_bounds.CenterPoint().y() -
                             kMenuButtonSize / 2);
  }

  menu_button_bounds.set_size(gfx::Size(kMenuButtonSize, kMenuButtonSize));
  menu_button_widget_->SetBounds(menu_button_bounds);
}

BEGIN_METADATA(SplitViewSeparator)
END_METADATA
