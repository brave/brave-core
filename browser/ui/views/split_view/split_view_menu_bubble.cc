/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_menu_bubble.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"

namespace {

SplitViewMenuBubble* g_bubble = nullptr;

constexpr auto kItemIconSize = 16;

class ItemView : public views::LabelButton {
  METADATA_HEADER(ItemView, LabelButton)
 public:
  ItemView() {
    auto* ink_drop = views::InkDrop::Get(this);
    ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
    ink_drop->SetBaseColorId(ui::kColorSysOnSurfaceSubtle);
    SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(0, 13)));

    image_container_view()->SetPreferredSize(
        gfx::Size(kItemIconSize, kItemIconSize));
    SetImageLabelSpacing(12);
  }
  ~ItemView() override = default;

  // views::LabelButton:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    auto size = views::LabelButton::CalculatePreferredSize(available_size);
    size.SetToMax(gfx::Size(0, 30));
    return size;
  }
};

BEGIN_METADATA(ItemView)
END_METADATA

BEGIN_VIEW_BUILDER(/*no export*/, ItemView, views::LabelButton)
END_VIEW_BUILDER

}  // namespace

// This should be out of anonymous namespace
DEFINE_VIEW_BUILDER(/*no export*/, ItemView)

// static
void SplitViewMenuBubble::Show(Browser* browser, views::View* anchor) {
  if (g_bubble) {
    g_bubble->GetWidget()->Close();
  }
  g_bubble = new SplitViewMenuBubble(browser, anchor);
  views::BubbleDialogDelegateView::CreateBubble(g_bubble)->Show();
}

SplitViewMenuBubble::SplitViewMenuBubble(Browser* browser, views::View* anchor)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_LEFT) {
  set_margins(gfx::Insets());
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
  SetButtons(ui::DIALOG_BUTTON_NONE);
  SetCloseCallback(
      base::BindOnce(&SplitViewMenuBubble::OnClose, base::Unretained(this)));

  auto browser_command_callback = [browser](int command_id) {
    return base::BindRepeating(
        [](Browser* browser, int command_id, const ui::Event& event) {
          chrome::ExecuteCommand(browser, command_id);
          if (!g_bubble) {
            return;
          }

          if (auto* widget = g_bubble->GetWidget();
              widget && !widget->IsClosed()) {
            widget->Close();
          }
        },
        browser, command_id);
  };

  auto get_image_model = [](const gfx::VectorIcon& icon) {
    return ui::ImageModel::FromVectorIcon(
        icon, kColorBraveSplitViewMenuItemIcon, kItemIconSize);
  };

  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser);
  const auto is_vertical_split =
      split_view_data->GetOrientation(
          browser->tab_strip_model()->GetActiveTab()->GetHandle()) ==
      SplitViewBrowserData::Orientation::kVertical;

  views::Builder<SplitViewMenuBubble>(this)
      .AddChild(views::Builder<ItemView>()
                    .SetText(l10n_util::GetStringUTF16(IDS_IDC_SWAP_SPLIT_VIEW))
                    .SetImageModel(views::Button::STATE_NORMAL,
                                   get_image_model(kLeoSwapHorizontalIcon))
                    .SetCallback(browser_command_callback(IDC_SWAP_SPLIT_VIEW)))
      .AddChild(
          views::Builder<ItemView>()
              .SetText(l10n_util::GetStringUTF16(IDS_IDC_BREAK_TILE))
              .SetImageModel(views::Button::STATE_NORMAL,
                             get_image_model(kLeoBrowserSplitViewUnsplitIcon))
              .SetCallback(browser_command_callback(IDC_BREAK_TILE)))
      .AddChild(views::Builder<ItemView>()
                    .SetText(l10n_util::GetStringUTF16(
                        is_vertical_split ? IDS_SPLIT_VIEW_SPLIT_HORIZONTAL
                                          : IDS_SPLIT_VIEW_SPLIT_VERTICAL))
                    // TODO(sko) Need Nala update
                    // .SetImageModel(views::Button::STATE_NORMAL,
                    //                get_image_model())
                    .SetCallback(browser_command_callback(
                        IDC_TOGGLE_SPLIT_VIEW_ORIENTATION)))
      .BuildChildren();
}

SplitViewMenuBubble::~SplitViewMenuBubble() = default;

void SplitViewMenuBubble::OnWidgetVisibilityChanged(views::Widget* widget,
                                                    bool visible) {
  if (!visible && !GetWidget()->IsClosed()) {
    GetWidget()->Close();
  }
}

void SplitViewMenuBubble::OnClose() {
  if (g_bubble == this) {
    g_bubble = nullptr;
  }
}

BEGIN_METADATA(SplitViewMenuBubble)
END_METADATA
