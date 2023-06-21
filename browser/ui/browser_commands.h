/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
#define BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_

#include "brave/components/playlist/common/buildflags/buildflags.h"

class Browser;
class GURL;

namespace brave {
bool HasSelectedURL(Browser* browser);
void CleanAndCopySelectedURL(Browser* browser);
void NewOffTheRecordWindowTor(Browser*);
void NewTorConnectionForSite(Browser*);
void AddNewProfile();
void OpenGuestProfile();
void ShowWalletBubble(Browser* browser);
void ShowApproveWalletBubble(Browser* browser);
void CloseWalletBubble(Browser* browser);
void MaybeDistillAndShowSpeedreaderBubble(Browser* browser);
void ShowBraveVPNBubble(Browser* browser);
void ToggleBraveVPNButton(Browser* browser);
void ToggleBraveVPNTrayIcon();
void OpenBraveVPNUrls(Browser* browser, int command_id);
void OpenIpfsFilesWebUI(Browser* browser);
// Copies an url sanitized by URLSanitizerService.
void CopySanitizedURL(Browser* browser, const GURL& url);
// Copies an url cleared through:
// - Debouncer (potentially debouncing many levels)
// - Query filter
// - URLSanitizerService
void CopyLinkWithStrictCleaning(Browser* browser, const GURL& url);

void ToggleWindowTitleVisibilityForVerticalTabs(Browser* browser);
void ToggleVerticalTabStrip(Browser* browser);
void ToggleVerticalTabStripFloatingMode(Browser* browser);
void ToggleActiveTabAudioMute(Browser* browser);
void ToggleSidebarPosition(Browser* browser);
void ToggleSidebar(Browser* browser);

void ToggleShieldsEnabled(Browser* browser);
void ToggleJavascriptEnabled(Browser* browser);

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
void ShowPlaylistBubble(Browser* browser);
#endif

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BROWSER_COMMANDS_H_
