// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../brave_appearance_page/sidebar.js'
import '../brave_appearance_page/tabs.js'
import '../brave_content_page/content.js'
import '../brave_content_page/playlist.js'
// <if expr="enable_speedreader">
import '../brave_content_page/speedreader.js'
// </if>
import '../brave_data_collection_page/brave_data_collection_page.js'
import '../brave_default_extensions_page/brave_default_extensions_page.js'
import '../brave_new_tab_page/brave_new_tab_page.js'
import '../brave_origin_page/brave_origin_page.js'
import '../brave_search_engines_page/brave_search_engines_page.js'
import '../brave_sync_page/brave_sync_page_index.js'
import '../brave_sync_page/brave_sync_manage_devices_page.js'
// <if expr="enable_tor">
import '../brave_tor_page/brave_tor_subpage.js'
// </if>
import '../brave_wallet_page/brave_wallet_page.js'
import '../brave_web3_domains_page/brave_web3_domains_page.js'
import '../default_brave_shields_page/default_brave_shields_page.js'
import '../getting_started_page/getting_started.js'
import '../social_blocking_page/social_blocking_page.js'

// <if expr="enable_ai_chat">
import '../brave_leo_assistant_page/brave_leo_assistant_page.js'
import '../brave_leo_assistant_page/model_list_section.js'
import '../brave_leo_assistant_page/personalization.js'
// </if>

// <if expr="enable_containers">
import '../brave_content_page/containers.js'
// </if>

import {
  html,
  RegisterStyleOverride,
} from 'chrome://resources/brave/polymer_overriding.js'


RegisterStyleOverride(
  'settings-basic-page',
  html`
    <style>
      :host {
        min-width: 544px !important;
      }
    </style>
  ` as HTMLTemplateElement
)

