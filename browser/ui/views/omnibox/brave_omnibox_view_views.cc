/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include <utility>

#include "brave/app/brave_command_ids.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_edit_controller.h"
#include "components/omnibox/browser/omnibox_edit_model.h"

namespace {
void BraveUpdateContextMenu(ui::SimpleMenuModel* menu_contents, GURL url) {
  if (!url.SchemeIsHTTPOrHTTPS())
    return;
  absl::optional<size_t> copy_position =
      menu_contents->GetIndexOfCommandId(views::Textfield::kCopy);
  if (!copy_position)
    return;
  menu_contents->InsertItemWithStringIdAt(
      copy_position.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
}
}  // namespace

BraveOmniboxViewViews::~BraveOmniboxViewViews() = default;

bool BraveOmniboxViewViews::SelectedTextIsURL() {
  GURL url;
  bool write_url = false;
  std::u16string selected_text = GetSelectedText();
  model()->AdjustTextForCopy(GetSelectedRange().GetMin(), &selected_text, &url,
                             &write_url);
  return write_url;
}

#if BUILDFLAG(IS_WIN)
bool BraveOmniboxViewViews::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  ui::KeyEvent event(
      accelerator.key_state() == ui::Accelerator::KeyState::PRESSED
          ? ui::ET_KEY_PRESSED
          : ui::ET_KEY_RELEASED,
      accelerator.key_code(), accelerator.modifiers());
  auto command = GetCommandForKeyEvent(event);

  if ((GetTextInputType() != ui::TEXT_INPUT_TYPE_PASSWORD) &&
      (command != ui::TextEditCommand::COPY || !SelectedTextIsURL())) {
    return OmniboxViewViews::AcceleratorPressed(accelerator);
  }
  ExecuteCommand(IDC_COPY_CLEAN_LINK, 0);
  return true;
}

bool BraveOmniboxViewViews::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  bool is_url = const_cast<BraveOmniboxViewViews*>(this)->SelectedTextIsURL();
  if (is_url) {
    if (command_id == kCopy) {
      return false;
    }
    if (command_id == IDC_COPY_CLEAN_LINK) {
      *accelerator = ui::Accelerator(ui::VKEY_C, ui::EF_PLATFORM_ACCELERATOR);
      return true;
    }
  }
  return OmniboxViewViews::GetAcceleratorForCommandId(command_id, accelerator);
}
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
void BraveOmniboxViewViews::ExecuteTextEditCommand(
    ui::TextEditCommand command) {
  if (command == ui::TextEditCommand::COPY && SelectedTextIsURL()) {
    ExecuteCommand(IDC_COPY_CLEAN_LINK, 0);
    return;
  }
  OmniboxViewViews::ExecuteTextEditCommand(command);
}
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

void BraveOmniboxViewViews::UpdateContextMenu(
    ui::SimpleMenuModel* menu_contents) {
  OmniboxViewViews::UpdateContextMenu(menu_contents);
  if (SelectedTextIsURL()) {
    BraveUpdateContextMenu(menu_contents,
                           controller()->GetLocationBarModel()->GetURL());
  }
}
