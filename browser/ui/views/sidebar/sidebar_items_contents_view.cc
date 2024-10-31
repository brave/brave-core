/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"

#include <optional>
#include <string>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/i18n/case_conversion.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/sidebar/sidebar_edit_item_bubble_delegate_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_added_feedback_bubble.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/event_utils.h"
#include "components/prefs/pref_service.h"
#include "ui/base/default_style.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/compositor/compositor.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/background.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"

namespace {

constexpr gfx::Size kIconSize(SidebarButtonView::kExternalIconSize,
                              SidebarButtonView::kExternalIconSize);

std::string GetFirstCharFromURL(const GURL& url) {
  DCHECK(url.is_valid());

  std::string target = url.host();
  if (target.empty()) {
    target = url.spec();
  }
  if (target.starts_with("www.")) {
    target = target.substr(4, 1);
  } else {
    target = target.substr(0, 1);
  }
  return target;
}

sidebar::SidebarService* GetSidebarService(Browser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

}  // namespace

SidebarItemsContentsView::SidebarItemsContentsView(
    BraveBrowser* browser,
    views::DragController* drag_controller)
    : browser_(browser),
      drag_controller_(drag_controller),
      sidebar_model_(browser->sidebar_controller()->model()) {
  DCHECK(browser_);
  set_context_menu_controller(this);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical))
      ->SetCollapseMarginsSpacing(true);
}

SidebarItemsContentsView::~SidebarItemsContentsView() = default;

gfx::Size SidebarItemsContentsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (children().empty()) {
    return {0, 0};
  }

  return views::View::CalculatePreferredSize(available_size);
}

void SidebarItemsContentsView::OnThemeChanged() {
  View::OnThemeChanged();

  // Skip when each item view is not attached.
  if (children().empty()) {
    return;
  }

  // Refresh favicons for web type items when theme changes.
  const auto& items = sidebar_model_->GetAllSidebarItems();
  const size_t items_num = items.size();
  CHECK_EQ(items_num, children().size()) << "Can contain only item view";

  for (size_t item_index = 0; item_index < items_num; ++item_index) {
    const auto item = items[item_index];
    if (!sidebar::IsWebType(item)) {
      continue;
    }

    SetDefaultImageFor(item);
    sidebar_model_->FetchFavicon(item);
  }
}

void SidebarItemsContentsView::Update() {
  UpdateAllBuiltInItemsViewState();
}

void SidebarItemsContentsView::UpdateAllBuiltInItemsViewState() {
  const auto& items = sidebar_model_->GetAllSidebarItems();
  // It's not initialized yet if child view count and items size are different.
  if (children().size() != items.size()) {
    return;
  }

  auto active_index = sidebar_model_->active_index();
  const size_t items_num = items.size();
  for (size_t item_index = 0; item_index < items_num; ++item_index) {
    const auto item = items[item_index];
    if (!sidebar::IsBuiltInType(item)) {
      continue;
    }

    // If browser window has tab that loads brave talk, brave talk panel icon
    // will use colored one for normal state also.
    if (item.built_in_item_type ==
        sidebar::SidebarItem::BuiltInItemType::kBraveTalk) {
      UpdateItemViewStateAt(
          item_index,
          browser_->sidebar_controller()->DoesBrowserHaveOpenedTabForItem(
              item));
      continue;
    }

    UpdateItemViewStateAt(item_index, item_index == active_index);
  }
}

void SidebarItemsContentsView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  if (context_menu_runner_ && context_menu_runner_->IsRunning()) {
    return;
  }

  if (!GetIndexOf(source)) {
    return;
  }

  view_for_context_menu_ = source;
  context_menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  SkColor icon_color = SK_ColorWHITE;
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    icon_color = color_provider->GetColor(kColorSidebarButtonBase);
  }
  context_menu_model_->AddItemWithIcon(
      kItemEdit,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ITEM_CONTEXT_MENU_EDIT),
      ui::ImageModel::FromVectorIcon(kSidebarEditIcon, icon_color, 14));
  context_menu_model_->AddItemWithIcon(
      kItemRemove,
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ITEM_CONTEXT_MENU_REMOVE),
      ui::ImageModel::FromVectorIcon(kSidebarTrashIcon, icon_color));
  context_menu_runner_ = std::make_unique<views::MenuRunner>(
      context_menu_model_.get(), views::MenuRunner::CONTEXT_MENU,
      base::BindRepeating(&SidebarItemsContentsView::OnContextMenuClosed,
                          base::Unretained(this)));
  context_menu_runner_->RunMenuAt(
      source->GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, source_type);
}

void SidebarItemsContentsView::LaunchEditItemDialog() {
  DCHECK(!observation_.IsObserving());
  DCHECK(view_for_context_menu_);
  auto index = GetIndexOf(view_for_context_menu_);
  const auto& items = sidebar_model_->GetAllSidebarItems();

  auto* bubble = SidebarEditItemBubbleDelegateView::Create(
      browser_, items[*index], view_for_context_menu_);
  observation_.Observe(bubble);
  bubble->Show();
}

void SidebarItemsContentsView::ExecuteCommand(int command_id, int event_flags) {
  auto index = GetIndexOf(view_for_context_menu_);
  if (command_id == kItemRemove) {
    GetSidebarService(browser_)->RemoveItemAt(*index);
    return;
  }

  if (command_id == kItemEdit) {
    LaunchEditItemDialog();
    return;
  }

  NOTREACHED_IN_MIGRATION();
}

bool SidebarItemsContentsView::IsCommandIdVisible(int command_id) const {
  auto index = GetIndexOf(view_for_context_menu_);
  const auto& items = sidebar_model_->GetAllSidebarItems();
  DCHECK(index < items.size());
  DCHECK(index);

  // Available for all items.
  if (command_id == kItemRemove) {
    return true;
  }

  if (command_id == kItemEdit) {
    return GetSidebarService(browser_)->IsEditableItemAt(*index);
  }

  NOTREACHED_IN_MIGRATION();
  return false;
}

void SidebarItemsContentsView::OnContextMenuClosed() {
  view_for_context_menu_ = nullptr;
}

void SidebarItemsContentsView::OnItemAdded(const sidebar::SidebarItem& item,
                                           int index,
                                           bool user_gesture) {
  AddItemView(item, index, user_gesture);
  InvalidateLayout();
}

void SidebarItemsContentsView::OnItemRemoved(int index) {
  RemoveChildViewT(children()[index]);
  InvalidateLayout();
}

void SidebarItemsContentsView::OnActiveIndexChanged(
    std::optional<size_t> old_index,
    std::optional<size_t> new_index) {
  if (old_index) {
    UpdateItemViewStateAt(*old_index, false);
  }

  if (new_index) {
    UpdateItemViewStateAt(*new_index, true);
  }
}

void SidebarItemsContentsView::OnItemMoved(const sidebar::SidebarItem& item,
                                           int from,
                                           int to) {
  views::View* source_view = children()[from];
  ReorderChildView(source_view, to);
}

void SidebarItemsContentsView::AddItemView(const sidebar::SidebarItem& item,
                                           int index,
                                           bool user_gesture) {
  auto* item_view =
      AddChildViewAt(std::make_unique<SidebarItemView>(
                         sidebar_model_->GetAllSidebarItems()[index].title),
                     index);
  item_view->set_context_menu_controller(this);
  item_view->SetCallback(
      base::BindRepeating(&SidebarItemsContentsView::OnItemPressed,
                          base::Unretained(this), item_view));
  item_view->set_drag_controller(drag_controller_);

  if (sidebar::IsWebType(item)) {
    SetDefaultImageFor(item);
  }

  UpdateItemViewStateAt(index, false);
}

void SidebarItemsContentsView::SetDefaultImageFor(
    const sidebar::SidebarItem& item) {
  SkColor text_color = SK_ColorWHITE;
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    text_color = color_provider->GetColor(kColorSidebarButtonBase);
  }

  const int scale = GetWidget()->GetCompositor()->device_scale_factor();
  gfx::Canvas canvas(kIconSize, scale, false);

  // TODO(simonhong): Ask this design to UX team for default image generation.
  // Use bigger font(8px larger than default for test) for text favicon.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  canvas.DrawStringRectWithFlags(
      base::i18n::ToUpper(base::UTF8ToUTF16(GetFirstCharFromURL(item.url))),
      rb.GetFontListWithDelta(8), text_color, gfx::Rect(kIconSize),
      gfx::Canvas::TEXT_ALIGN_CENTER);

  SetImageForItem(item,
                  gfx::ImageSkia(gfx::ImageSkiaRep(canvas.GetBitmap(), scale)));
}

void SidebarItemsContentsView::UpdateItem(
    const sidebar::SidebarItem& item,
    const sidebar::SidebarItemUpdate& update) {
  //  Set default for new url. Then waiting favicon update event.
  if (update.url_updated) {
    SetDefaultImageFor(item);
  }

  // Each item button uses accessible name as a title.
  if (update.title_updated) {
    auto title = item.title;
    if (title.empty()) {
      title = base::UTF8ToUTF16(item.url.spec());
    }
    GetItemViewAt(update.index)->SetAccessibleName(title);
  }
}

void SidebarItemsContentsView::ShowItemAddedFeedbackBubble(
    size_t item_added_index) {
  auto* prefs = browser_->profile()->GetPrefs();
  const int current_count =
      prefs->GetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount);
  // Don't show feedback bubble more than three times.
  if (current_count >= 3) {
    return;
  }
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount,
                    current_count + 1);
  CHECK_LT(item_added_index, children().size());
  views::View* lastly_added_view = children()[item_added_index];
  ShowItemAddedFeedbackBubble(lastly_added_view);
}

void SidebarItemsContentsView::ShowItemAddedFeedbackBubble(
    views::View* anchor_view) {
  // Only launch feedback bubble for active browser window.
  DCHECK_EQ(browser_, BrowserList::GetInstance()->GetLastActive());
  DCHECK(!observation_.IsObserving());

  if (item_added_bubble_launched_for_test_) {
    // Early return w/o launching actual bubble for quick test.
    CHECK_IS_TEST();
    item_added_bubble_launched_for_test_.Run(anchor_view);
    return;
  }

  auto* bubble = SidebarItemAddedFeedbackBubble::Create(anchor_view, this);
  observation_.Observe(bubble);
  bubble->Show();
}

bool SidebarItemsContentsView::IsBuiltInTypeItemView(views::View* view) const {
  auto index = GetIndexOf(view);
  return sidebar::IsBuiltInType(sidebar_model_->GetAllSidebarItems()[*index]);
}

void SidebarItemsContentsView::SetImageForItem(const sidebar::SidebarItem& item,
                                               const gfx::ImageSkia& image) {
  auto index = sidebar_model_->GetIndexOf(item);
  // disengaged means |item| is deleted while fetching favicon.
  if (!index) {
    return;
  }
  CHECK_LT(*index, children().size());

  SidebarItemView* item_view = GetItemViewAt(*index);
  item_view->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromImageSkia(
          gfx::ImageSkiaOperations::CreateResizedImage(
              image, skia::ImageOperations::RESIZE_BEST, kIconSize)));
}

void SidebarItemsContentsView::ClearDragIndicator() {
  for (views::View* view : children()) {
    static_cast<SidebarItemView*>(view)->ClearHorizontalBorder();
  }
}

std::optional<size_t>
SidebarItemsContentsView::CalculateTargetDragIndicatorIndex(
    const gfx::Point& screen_position) {
  // Find which item view includes this |screen_position|.
  const int child_count = children().size();
  for (int i = 0; i < child_count; ++i) {
    views::View* child_view = children()[i];
    gfx::Rect child_view_rect = child_view->GetLocalBounds();
    views::View::ConvertRectToScreen(child_view, &child_view_rect);
    // We use |SidebarButtonView::kMargin|px for spacing between items.
    // This spacing should be considered as each item's area to know
    // which item contains |screen_position|. Added half of margin
    // to items' top & bottom. For first and last items, includes whole
    // margin to its top & bottom.
    const bool is_first_item = (i == 0);
    const bool is_last_item = (i == (child_count - 1));

    // Re-visit when |kMargin| is odd number.
    CHECK_EQ(0, SidebarButtonView::kMargin % 2);
    child_view_rect.Outset(
        gfx::Outsets::TLBR(is_first_item ? SidebarButtonView::kMargin
                                         : SidebarButtonView::kMargin / 2,
                           0,
                           is_last_item ? SidebarButtonView::kMargin
                                        : SidebarButtonView::kMargin / 2,
                           0));

    if (child_view_rect.Contains(screen_position)) {
      const gfx::Point center_point = child_view_rect.CenterPoint();
      return center_point.y() > screen_position.y() ? i : i + 1;
    }
  }

  NOTREACHED_IN_MIGRATION();
  return std::nullopt;
}

std::optional<size_t> SidebarItemsContentsView::DrawDragIndicator(
    views::View* source,
    const gfx::Point& position) {
  auto source_view_index = GetIndexOf(source);
  auto target_index = CalculateTargetDragIndicatorIndex(position);
  // If target position is right before or right after, don't need to draw
  // drag indicator.
  DCHECK(source_view_index);
  if (source_view_index == target_index ||
      *source_view_index + 1 == target_index) {
    ClearDragIndicator();
  } else {
    DoDrawDragIndicator(target_index);
  }

  return target_index;
}

void SidebarItemsContentsView::DoDrawDragIndicator(
    std::optional<size_t> index) {
  // Clear current drag indicator.
  ClearDragIndicator();

  if (!index) {
    return;
  }

  // Use item's top or bottom border as a drag indicator.
  // Item's top border is used as a drag indicator except last item.
  // Last item's bottom border is used for indicator when drag candidate
  // position is behind the last item.
  const size_t child_count = children().size();
  const bool draw_top_border = child_count != *index;
  const size_t item_index = draw_top_border ? *index : *index - 1;
  GetItemViewAt(item_index)->DrawHorizontalBorder(draw_top_border);
}

SidebarItemView* SidebarItemsContentsView::GetItemViewAt(size_t index) {
  return static_cast<SidebarItemView*>(children()[index]);
}

void SidebarItemsContentsView::UpdateItemViewStateAt(size_t index,
                                                     bool active) {
  const auto& item = sidebar_model_->GetAllSidebarItems()[index];
  SidebarItemView* item_view = GetItemViewAt(index);

  if (item.open_in_panel) {
    item_view->SetActiveState(active);
  }

  if (sidebar::IsBuiltInType(item)) {
    for (const auto state : views::Button::kButtonStates) {
      auto color_state = state;
      if (active && state != views::Button::STATE_DISABLED) {
        color_state = views::Button::STATE_PRESSED;
      }

      item_view->SetImageModel(
          state, GetImageForBuiltInItems(item.built_in_item_type, color_state));
    }
  }
}

void SidebarItemsContentsView::OnItemPressed(const views::View* item,
                                             const ui::Event& event) {
  auto* controller = browser_->sidebar_controller();
  auto index = GetIndexOf(item);
  if (controller->IsActiveIndex(index)) {
    controller->DeactivateCurrentPanel();
    return;
  }

  const auto& item_model = controller->model()->GetAllSidebarItems()[*index];
  if (item_model.open_in_panel) {
    if (item_model.built_in_item_type ==
        sidebar::SidebarItem::BuiltInItemType::kChatUI) {
      ai_chat::AIChatMetrics* metrics =
          g_brave_browser_process->process_misc_metrics()->ai_chat_metrics();
      CHECK(metrics);
      metrics->HandleOpenViaEntryPoint(ai_chat::EntryPoint::kSidebar);
    }
    controller->ActivatePanelItem(item_model.built_in_item_type);
    return;
  }

  WindowOpenDisposition open_disposition = WindowOpenDisposition::CURRENT_TAB;
  if (event_utils::IsPossibleDispositionEvent(event)) {
    open_disposition = ui::DispositionFromEventFlags(event.flags());
  }

  controller->ActivateItemAt(index, open_disposition);
}

ui::ImageModel SidebarItemsContentsView::GetImageForBuiltInItems(
    sidebar::SidebarItem::BuiltInItemType type,
    views::Button::ButtonState state) const {
  const auto get_image_model = [](const gfx::VectorIcon& icon,
                                  views::Button::ButtonState state) {
    return ui::ImageModel::FromVectorIcon(
        icon,
        state == views::Button::STATE_DISABLED
            ? kColorSidebarArrowDisabled
            : (state == views::Button::STATE_PRESSED
                   ? kColorSidebarButtonPressed
                   : kColorSidebarButtonBase),
        SidebarButtonView::kDefaultIconSize);
  };

  switch (type) {
    case sidebar::SidebarItem::BuiltInItemType::kWallet:
      return get_image_model(kLeoProductBraveWalletIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kBraveTalk:
      return get_image_model(kLeoProductBraveTalkIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kBookmarks:
      return get_image_model(kLeoProductBookmarksIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kReadingList:
      return get_image_model(kLeoReadingListIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kHistory:
      return get_image_model(kLeoHistoryIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kPlaylist:
      return get_image_model(kLeoProductPlaylistIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kChatUI:
      return get_image_model(kLeoProductBraveLeoIcon, state);
    case sidebar::SidebarItem::BuiltInItemType::kNone:
      NOTREACHED_NORETURN();
  }
}

void SidebarItemsContentsView::OnWidgetDestroying(views::Widget* widget) {
  observation_.Reset();
}

bool SidebarItemsContentsView::IsBubbleVisible() const {
  if (context_menu_runner_ && context_menu_runner_->IsRunning()) {
    return true;
  }

  if (observation_.IsObserving()) {
    return true;
  }

  return false;
}

BEGIN_METADATA(SidebarItemsContentsView)
END_METADATA
