/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_context_menu/brave_spelling_options_submenu_observer.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/grit/generated_resources.h"
#include "components/renderer_context_menu/render_view_context_menu_proxy.h"
#include "ui/base/l10n/l10n_util.h"

BraveSpellingOptionsSubMenuObserver::BraveSpellingOptionsSubMenuObserver(
    RenderViewContextMenuProxy* proxy,
    ui::SimpleMenuModel::Delegate* delegate,
    int group_id)
    : SpellingOptionsSubMenuObserver(proxy, delegate, group_id),
      gtest_mode_(GTEST_MODE_DISABLED) {}

void BraveSpellingOptionsSubMenuObserver::InitMenu(
    const content::ContextMenuParams& params) {
  // Let Chromium build the submenu.
  SpellingOptionsSubMenuObserver::InitMenu(params);

  // Assumptions:
  // 1. Use of spelling service is disabled in Brave profile preferences.
  // 2. We overrode RenderViewContextMenu::AddSpellCheckServiceItem so that the
  // spelling suggestions toggle isn't added to the menu by the base class.
  DCHECK(!use_spelling_service_.GetValue());
  DCHECK(submenu_model_.GetIndexOfCommandId(
             IDC_CONTENT_CONTEXT_SPELLING_TOGGLE) == -1);

  // Check if we ended up with a separator as the last item and, if so, get rid
  // of it.
  int index = submenu_model_.GetItemCount() - 1;
  if (index >= 0 &&
      submenu_model_.GetTypeAt(index) == ui::MenuModel::TYPE_SEPARATOR) {
    submenu_model_.RemoveItemAt(index);
  }
  DCHECK(submenu_model_.GetItemCount());

  // Special accommodations for gtest.
  if (gtest_mode_ != GTEST_MODE_DISABLED) {
    if (gtest_mode_ == GTEST_MODE_EMPTY_SUBMENU) {
      // Simulate empty submenu situation to test the UpdateMenuItem code below.
      submenu_model_.Clear();
    }
    // In browser tests, the mock menu item doesn't store the submenu_model_
    // pointer and instead flattens the menu into a vector in AddSubmenuItem,
    // which means we need to update the proxy manually.
    proxy_->RemoveMenuItem(IDC_SPELLCHECK_MENU);
    proxy_->AddSubMenu(
        IDC_SPELLCHECK_MENU,
        l10n_util::GetStringUTF16(IDS_CONTENT_CONTEXT_SPELLCHECK_MENU),
        &submenu_model_);
  }

  // If somehow we ended up with an empty submenu then disable it.
  if (!submenu_model_.GetItemCount())
    proxy_->UpdateMenuItem(
        IDC_SPELLCHECK_MENU,
        false,  // enabled
        false,  // hidden
        l10n_util::GetStringUTF16(IDS_CONTENT_CONTEXT_SPELLCHECK_MENU));
}

void BraveSpellingOptionsSubMenuObserver::SetGtestMode(
    BraveSpellingOptionsSubMenuObserver::GtestMode mode) {
  gtest_mode_ = mode;
}
