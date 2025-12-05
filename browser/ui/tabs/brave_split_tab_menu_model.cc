/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_split_tab_menu_model.h"

#include <memory>

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/tabs/split_tab_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/split_tab_id.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"

std::unique_ptr<ui::SimpleMenuModel> CreateBraveSplitTabMenuModel(
    TabStripModel* tab_strip_model,
    SplitTabMenuModel::MenuSource source) {
  return std::make_unique<BraveSplitTabMenuModel>(tab_strip_model, source);
}

BraveSplitTabMenuModel::BraveSplitTabMenuModel(
    TabStripModel* tab_strip_model,
    MenuSource menu_source,
    std::optional<int> split_tab_index)
    : SplitTabMenuModel(tab_strip_model, menu_source, split_tab_index) {
  // Remove "Send feedback" and the separator above it.
  auto feedback_command_index =
      GetIndexOfCommandId(GetCommandIdInt(CommandId::kSendFeedback));
  if (feedback_command_index.has_value()) {
    CHECK(feedback_command_index.value() > 1);
    auto separator_index = feedback_command_index.value() - 1;
    CHECK(GetSeparatorTypeAt(separator_index) ==
          ui::MenuSeparatorType::NORMAL_SEPARATOR);
    RemoveItemAt(feedback_command_index.value());
    RemoveItemAt(separator_index);
  }

  AddItem(GetCommandIdInt(CommandId::kToggleLinkState), std::u16string());
}

BraveSplitTabMenuModel::~BraveSplitTabMenuModel() = default;

bool BraveSplitTabMenuModel::IsItemForCommandIdDynamic(int command_id) const {
  const CommandId id = GetCommandIdEnum(command_id);

  // It's not dynamic but handle like that to apply our label/icon via GetXXX().
  if (id == CommandId::kExitSplit) {
    return true;
  }

  // Icon/string can be changed whether it's link or unlink.
  if (id == CommandId::kToggleLinkState) {
    return true;
  }

  return SplitTabMenuModel::IsItemForCommandIdDynamic(command_id);
}

std::u16string BraveSplitTabMenuModel::GetLabelForCommandId(
    int command_id) const {
  const CommandId id = GetCommandIdEnum(command_id);

  if (id == CommandId::kReversePosition) {
    return l10n_util::GetStringUTF16(IDS_IDC_SWAP_SPLIT_VIEW);
  }

  if (id == CommandId::kExitSplit) {
    return l10n_util::GetStringUTF16(IDS_IDC_BREAK_TILE);
  }

  if (id == CommandId::kToggleLinkState) {
    const split_tabs::SplitTabId split_id = GetSplitTabId();
    split_tabs::SplitTabData* const split_tab_data =
        tab_strip_model_->GetSplitData(split_id);
    const bool linked = split_tab_data->linked();
    return l10n_util::GetStringUTF16(linked ? IDS_IDC_SPLIT_VIEW_UNLINK
                                            : IDS_IDC_SPLIT_VIEW_LINK);
  }

  return SplitTabMenuModel::GetLabelForCommandId(command_id);
}

ui::ImageModel BraveSplitTabMenuModel::GetIconForCommandId(
    int command_id) const {
  const CommandId id = GetCommandIdEnum(command_id);

  if (id == CommandId::kExitSplit) {
    return ui::ImageModel::FromVectorIcon(
        kLeoBrowserSplitViewUnsplitIcon, ui::kColorMenuIcon,
        ui::SimpleMenuModel::kDefaultIconSize);
  }

  if (id == CommandId::kToggleLinkState) {
    const split_tabs::SplitTabId split_id = GetSplitTabId();
    split_tabs::SplitTabData* const split_tab_data =
        tab_strip_model_->GetSplitData(split_id);
    const bool linked = split_tab_data->linked();
    return ui::ImageModel::FromVectorIcon(
        linked ? kLeoLinkBrokenIcon : kLeoLinkNormalIcon, ui::kColorMenuIcon,
        ui::SimpleMenuModel::kDefaultIconSize);
  }

  return SplitTabMenuModel::GetIconForCommandId(command_id);
}

const gfx::VectorIcon& BraveSplitTabMenuModel::GetReversePositionIcon(
    split_tabs::SplitTabActiveLocation active_split_tab_location) const {
  switch (active_split_tab_location) {
    case split_tabs::SplitTabActiveLocation::kStart:
      [[fallthrough]];
    case split_tabs::SplitTabActiveLocation::kEnd:
      return kLeoSwapHorizontalIcon;
    default:
      return SplitTabMenuModel::GetReversePositionIcon(
          active_split_tab_location);
  }
}
