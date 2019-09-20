// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/browser/renderer_context_menu/brave_spelling_options_submenu_observer.h"

// Our .h file creates a masquerade for RenderViewContextMenu.  Switch
// back to the Chromium one for the Chromium implementation.
#undef RenderViewContextMenu
#define RenderViewContextMenu RenderViewContextMenu_Chromium

// Use our subclass to initialize SpellingOptionsSubMenuObserver.
#define SpellingOptionsSubMenuObserver BraveSpellingOptionsSubMenuObserver

#include "../../../../chrome/browser/renderer_context_menu/render_view_context_menu.cc"  // NOLINT

#undef SpellingOptionsSubMenuObserver

// Make it clear which class we mean here.
#undef RenderViewContextMenu

BraveRenderViewContextMenu::BraveRenderViewContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params)
  : RenderViewContextMenu_Chromium(render_frame_host, params) {
}

bool BraveRenderViewContextMenu::IsCommandIdEnabled(int id) const {
  switch (id) {
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
#if BUILDFLAG(ENABLE_TOR)
      return params_.link_url.is_valid() &&
             IsURLAllowedInIncognito(params_.link_url, browser_context_) &&
             !brave::IsTorProfile(GetProfile());
#else
      return false;
#endif
    default:
      return RenderViewContextMenu_Chromium::IsCommandIdEnabled(id);
  }
}

void BraveRenderViewContextMenu::ExecuteCommand(int id, int event_flags) {
  switch (id) {
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
      profiles::SwitchToTorProfile(
          base::Bind(
              OnProfileCreated, params_.link_url,
              content::Referrer(
                GURL(), network::mojom::ReferrerPolicy::kStrictOrigin)));
      break;
    default:
      RenderViewContextMenu_Chromium::ExecuteCommand(id, event_flags);
  }
}

void BraveRenderViewContextMenu::AddSpellCheckServiceItem(bool is_checked) {
  // Call our implementation, not the one in the base class.
  // Assumption:
  // Use of spelling service is disabled in Brave profile preferences.
  DCHECK(!GetProfile()->GetPrefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService));
  AddSpellCheckServiceItem(&menu_model_, is_checked);
}

// static
void BraveRenderViewContextMenu::AddSpellCheckServiceItem(
    ui::SimpleMenuModel* menu,
    bool is_checked) {
  // Suppress adding "Spellcheck->Ask Brave for suggestions" item.
}

void BraveRenderViewContextMenu::InitMenu() {
  RenderViewContextMenu_Chromium::InitMenu();

  // Add Open Link with Tor
  int index = -1;
  if (!params_.link_url.is_empty()) {
    const Browser* browser = GetBrowser();
    const bool is_app = browser && browser->is_type_app();

    index = menu_model_.GetIndexOfCommandId(
        IDC_CONTENT_CONTEXT_OPENLINKOFFTHERECORD);
    DCHECK_NE(index, -1);

    menu_model_.InsertItemWithStringIdAt(
        index + 1,
        IDC_CONTENT_CONTEXT_OPENLINKTOR,
        is_app ? IDS_CONTENT_CONTEXT_OPENLINKTOR_INAPP
               : IDS_CONTENT_CONTEXT_OPENLINKTOR);
  }

  // Only show the translate item when go-translate is enabled.
#if !BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  index = menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_TRANSLATE);
  if (index != -1)
    menu_model_.RemoveItemAt(index);
#endif
}
