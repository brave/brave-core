/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_menu_bubble.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"

namespace {

SplitViewMenuBubble* g_bubble = nullptr;

class ItemView : public views::LabelButton {
  METADATA_HEADER(ItemView, LabelButton)
 public:
  ItemView() {
    SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(0, 13)));
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
  set_margins(gfx::Insets::VH(4, 0));
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  SetCloseCallback(
      base::BindOnce(&SplitViewMenuBubble::OnClose, base::Unretained(this)));

  auto browser_command_callback = [browser](int command_id) {
    return base::BindRepeating(
        [](Browser* browser, int command_id, const ui::Event& event) {
          chrome::ExecuteCommand(browser, command_id);
          // Some platforms steal this bubble's focus closing
          // implicitly but some don't, so we need to close it explicitly.
          if (g_bubble) {
            g_bubble->GetWidget()->Close();
          }
        },
        browser, command_id);
  };

  auto get_image_model = [](const gfx::VectorIcon& icon) {
    return ui::ImageModel::FromVectorIcon(icon,
                                          kColorBraveSplitViewMenuItemIcon, 16);
  };

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
