// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/ai_rewriter/ai_rewriter_button_view.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_rewriter/ai_rewriter_button.h"
#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

namespace ai_rewriter {

namespace {
constexpr int kButtonRadius = 12;
}

AIRewriterButtonView::AIRewriterButtonView(Browser* browser,
                                           content::WebContents* contents)
    : content::WebContentsObserver(contents) {
  tab_strip_observation_.Observe(browser->tab_strip_model());

  views::Builder<AIRewriterButtonView>(this)
      .SetBackground(
          views::CreateRoundedRectBackground(SK_ColorWHITE, kButtonRadius))
      .SetBorder(views::CreateRoundedRectBorder(1, kButtonRadius, SK_ColorGRAY))
      .SetLayoutManager(std::make_unique<views::FillLayout>())
      .AddChild(
          views::Builder<views::LabelButton>()
              .SetImageModel(
                  views::LabelButton::ButtonState::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(kLeoProductBraveLeoIcon))
              .SetImageModel(views::LabelButton::ButtonState::STATE_HOVERED,
                             ui::ImageModel::FromVectorIcon(
                                 kLeoProductBraveLeoIcon,
                                 ui::ColorIds::kColorButtonForeground))
              .SetPreferredSize(gfx::Size(32, 32))
              .SetCallback(base::BindRepeating(
                  base::IgnoreResult(&AIRewriterButtonView::OpenDialog),
                  base::Unretained(this))))
      .BuildChildren();
}

AIRewriterButtonView::~AIRewriterButtonView() = default;

base::WeakPtr<AIRewriterButton> AIRewriterButtonView::MaybeCreateButton(
    content::WebContents* contents) {
  auto* browser = chrome::FindBrowserWithTab(contents);

  // Possible for non-tab WebContents
  if (!browser) {
    return nullptr;
  }

  auto* button = new AIRewriterButtonView(browser, contents);

  auto* parent_widget = views::Widget::GetWidgetForNativeWindow(
      browser->window()->GetNativeWindow());
  CHECK(parent_widget);

  views::Widget::InitParams params(
      views::Widget::InitParams::Type::TYPE_CONTROL);
  params.parent = parent_widget->GetNativeView();
  params.activatable = views::Widget::InitParams::Activatable::kNo;
  params.delegate = button;
  params.shadow_type = views::Widget::InitParams::ShadowType::kDrop;
  params.corner_radius = kButtonRadius;
  params.autosize = true;

  auto* widget = new views::Widget();
  widget->Init(std::move(params));
  widget->Hide();

  return button->weak_ptr_factory_.GetWeakPtr();
}

void AIRewriterButtonView::Show(const gfx::Rect& rect) {
  CHECK(GetWidget());
  GetWidget()->Show();

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(
      chrome::FindBrowserWithTab(web_contents()));
  CHECK(browser_view);

  auto offset = browser_view->contents_container()->bounds().OffsetFromOrigin();

  constexpr int kPaddingY = -8;
  auto size = GetPreferredSize();
  auto pos = rect.origin() + offset;
  pos.Offset(GetPreferredSize().width() / 2,
             -GetPreferredSize().height() / 2 + kPaddingY);
  GetWidget()->SetBounds(gfx::Rect(pos, size));
}

void AIRewriterButtonView::Hide() {
  GetWidget()->Hide();
}

void AIRewriterButtonView::Close() {
  GetWidget()->Close();
}

bool AIRewriterButtonView::IsShowing() const {
  return GetWidget()->IsVisible();
}

ui::WebDialogDelegate* AIRewriterButtonView::OpenDialog() {
  auto* host = web_contents()->GetFocusedFrame()->GetRenderWidgetHost();
  CHECK(host);

  auto* host_view = host->GetView();
  if (!host_view) {
    return nullptr;
  }

  auto selected = host_view->GetSelectedText();
  return AIRewriterDialogDelegate::Show(web_contents(),
                                        base::UTF16ToUTF8(selected));
}

void AIRewriterButtonView::PrimaryPageChanged(content::Page& page) {
  Close();
}

void AIRewriterButtonView::WebContentsDestroyed() {
  Close();
}

void AIRewriterButtonView::OnVisibilityChanged(content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN) {
    Hide();
  }
}

void AIRewriterButtonView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  // If the tab has been removed, close the widget
  if (change.type() == TabStripModelChange::kRemoved &&
      base::Contains(change.GetRemove()->contents, web_contents(),
                     &TabStripModelChange::RemovedTab::contents)) {
    Close();
  }
}

base::WeakPtr<AIRewriterButton> CreateRewriterButton(
    content::WebContents* contents) {
  return AIRewriterButtonView::MaybeCreateButton(contents);
}

BEGIN_METADATA(AIRewriterButtonView)
END_METADATA

}  // namespace ai_rewriter
