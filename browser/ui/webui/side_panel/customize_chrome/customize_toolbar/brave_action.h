// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_BRAVE_ACTION_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_BRAVE_ACTION_H_

#include "base/containers/fixed_flat_map.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/gfx/vector_icon_types.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

namespace customize_chrome {

// This struct holds information about each Brave specific action that will be
// added to the toolbar customization list. Not only it contains the UI data,
// but also the pref name that controls the visibility of the action.
struct BraveAction {
  const side_panel::customize_chrome::mojom::ActionId id;
  const int display_name_resource_id;
  const side_panel::customize_chrome::mojom::ActionId anchor;
  const side_panel::customize_chrome::mojom::CategoryId category;
  const char* pref_name;
  // RAW_PTR_EXCLUSION: #global-scope
  RAW_PTR_EXCLUSION const gfx::VectorIcon& icon;
};

inline constexpr BraveAction kShowSidePanelAction = {
    .id = side_panel::customize_chrome::mojom::ActionId::kShowSidePanel,
    .display_name_resource_id = IDS_CUSTOMIZE_TOOLBAR_TOGGLE_SIDEBAR,
    .anchor =
        side_panel::customize_chrome::mojom::ActionId::kNewIncognitoWindow,
    .category = side_panel::customize_chrome::mojom::CategoryId::kNavigation,
    .pref_name = kShowSidePanelButton,
    .icon = kLeoBrowserSidebarRightIcon};

inline constexpr BraveAction kShowWalletAction = {
    .id = side_panel::customize_chrome::mojom::ActionId::kShowWallet,
    .display_name_resource_id = IDS_CUSTOMIZE_TOOLBAR_TOGGLE_WALLET,
    .anchor = side_panel::customize_chrome::mojom::ActionId::kTabSearch,
    .category = side_panel::customize_chrome::mojom::CategoryId::kNavigation,
    .pref_name = kShowWalletIconOnToolbar,
    .icon = kLeoProductBraveWalletIcon};

inline constexpr BraveAction kShowAIChatAction = {
    .id = side_panel::customize_chrome::mojom::ActionId::kShowAIChat,
    .display_name_resource_id = IDS_CUSTOMIZE_TOOLBAR_TOGGLE_AI_CHAT,
    .anchor = side_panel::customize_chrome::mojom::ActionId::kTabSearch,
    .category = side_panel::customize_chrome::mojom::CategoryId::kNavigation,
    .pref_name = ai_chat::prefs::kBraveAIChatShowToolbarButton,
    .icon = kLeoProductBraveLeoIcon};

#if BUILDFLAG(ENABLE_BRAVE_VPN)
inline constexpr BraveAction kShowVPNAction = {
    .id = side_panel::customize_chrome::mojom::ActionId::kShowVPN,
    .display_name_resource_id = IDS_CUSTOMIZE_TOOLBAR_TOGGLE_VPN,
    .anchor = side_panel::customize_chrome::mojom::ActionId::kTabSearch,
    .category = side_panel::customize_chrome::mojom::CategoryId::kNavigation,
    .pref_name = brave_vpn::prefs::kBraveVPNShowButton,
    .icon = kLeoProductVpnIcon};
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

inline constexpr auto kBraveActions =
    base::MakeFixedFlatMap<side_panel::customize_chrome::mojom::ActionId,
                           const BraveAction*>({
        {kShowSidePanelAction.id, &kShowSidePanelAction},
        {kShowWalletAction.id, &kShowWalletAction},
        {kShowAIChatAction.id, &kShowAIChatAction},
#if BUILDFLAG(ENABLE_BRAVE_VPN)
        {kShowVPNAction.id, &kShowVPNAction},
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
    });

}  // namespace customize_chrome

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_BRAVE_ACTION_H_
