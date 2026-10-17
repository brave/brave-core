// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Registers the page elements that the settings_main.html lit_mangler adds to
// the view manager.
import '../brave_content_page/content_page_index.js'
import '../getting_started_page/getting_started_page_index.js'
import '../brave_origin_page/brave_origin_page.js'
import '../default_brave_shields_page/shields_page_index.js'
import '../brave_default_extensions_page/brave_extensions_page_index.js'
import '../brave_sync_page/brave_sync_page_index.js'
import '../brave_system_page/brave_system_page_index.js'

// <if expr="enable_ai_chat">
import '../brave_leo_assistant_page/brave_leo_assistant_page_index.js'
// </if>

// <if expr="enable_brave_wallet">
import '../brave_wallet_page/wallet_page_index.js'
// </if>
