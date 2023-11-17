/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include <optional>
#include <utility>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "ui/base/metadata/metadata_impl_macros.h"

namespace {
void BraveUpdateContextMenu(ui::SimpleMenuModel* menu_contents, GURL url) {
  if (!url.SchemeIsHTTPOrHTTPS())
    return;
  std::optional<size_t> copy_position =
      menu_contents->GetIndexOfCommandId(views::Textfield::kCopy);
  if (!copy_position)
    return;
  menu_contents->InsertItemWithStringIdAt(
      copy_position.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
}
}  // namespace

BraveOmniboxViewViews::~BraveOmniboxViewViews() = default;

std::optional<GURL> BraveOmniboxViewViews::GetURLToCopy() {
  GURL url;
  bool write_url = false;
  std::u16string selected_text = GetSelectedText();
  model()->AdjustTextForCopy(GetSelectedRange().GetMin(), &selected_text, &url,
                             &write_url);
  if (!write_url) {
    return std::nullopt;
  }
  return url;
}

bool BraveOmniboxViewViews::SelectedTextIsURL() {
  return GetURLToCopy().has_value();
}

void BraveOmniboxViewViews::CleanAndCopySelectedURL() {
  auto url_to_copy = GetURLToCopy();
  if (!url_to_copy.has_value()) {
    return;
  }
  CopySanitizedURL(url_to_copy.value());
}

void BraveOmniboxViewViews::CopySanitizedURL(const GURL& url) {
  OnBeforePossibleChange();
  brave::CopySanitizedURL(chrome::FindLastActive(), url);
  OnAfterPossibleChange(true);
}

#if BUILDFLAG(IS_WIN)
bool BraveOmniboxViewViews::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  if (!base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkByDefault)) {
    return OmniboxViewViews::AcceleratorPressed(accelerator);
  }

  ui::KeyEvent event(
      accelerator.key_state() == ui::Accelerator::KeyState::PRESSED
          ? ui::ET_KEY_PRESSED
          : ui::ET_KEY_RELEASED,
      accelerator.key_code(), accelerator.modifiers());
  auto command = GetCommandForKeyEvent(event);
  auto url_to_copy = GetURLToCopy();
  if ((GetTextInputType() != ui::TEXT_INPUT_TYPE_PASSWORD) &&
      (command != ui::TextEditCommand::COPY)) {
    return OmniboxViewViews::AcceleratorPressed(accelerator);
  }
  if (!url_to_copy.has_value()) {
    return OmniboxViewViews::AcceleratorPressed(accelerator);
  }
  CopySanitizedURL(url_to_copy.value());
  return true;
}
#endif  // BUILDFLAG(IS_WIN)

bool BraveOmniboxViewViews::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  if (base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkByDefault)) {
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
  }
  return OmniboxViewViews::GetAcceleratorForCommandId(command_id, accelerator);
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
void BraveOmniboxViewViews::ExecuteTextEditCommand(
    ui::TextEditCommand command) {
  if (base::FeatureList::IsEnabled(features::kBraveCopyCleanLinkByDefault)) {
    auto url_to_copy = GetURLToCopy();
    if (command == ui::TextEditCommand::COPY && url_to_copy.has_value()) {
      CopySanitizedURL(url_to_copy.value());
      return;
    }
  }
  OmniboxViewViews::ExecuteTextEditCommand(command);
}
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

void BraveOmniboxViewViews::UpdateContextMenu(
    ui::SimpleMenuModel* menu_contents) {
  OmniboxViewViews::UpdateContextMenu(menu_contents);
  auto url_to_copy = GetURLToCopy();
  if (url_to_copy.has_value()) {
    BraveUpdateContextMenu(menu_contents, url_to_copy.value());
  }
}

BEGIN_METADATA(BraveOmniboxViewViews)
END_METADATA
