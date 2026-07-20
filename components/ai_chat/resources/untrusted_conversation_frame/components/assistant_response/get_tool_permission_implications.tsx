// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  getLocale,
  // <if expr="enable_ai_chat_tab_management_tool">
  formatLocale,
  // </if>
} from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
// <if expr="enable_ai_chat_tab_management_tool">
import * as React from 'react'
// </if>

export function getToolPermissionImplications(toolName: string) {
  switch (toolName) {
    case Mojom.SEMANTIC_HISTORY_SEARCH_TOOL_NAME:
      return getLocale(
        S.CHAT_UI_TOOL_SEMANTIC_HISTORY_SEARCH_PERMISSION_IMPLICATIONS,
      )
    case Mojom.SEMANTIC_TAB_SEARCH_TOOL_NAME:
      return getLocale(
        S.CHAT_UI_TOOL_SEMANTIC_TAB_SEARCH_PERMISSION_IMPLICATIONS,
      )
    // <if expr="enable_ai_chat_tab_management_tool">
    case Mojom.TAB_MANAGEMENT_TOOL_NAME:
      return formatLocale(
        S.CHAT_UI_TOOL_TAB_MANAGEMENT_PERMISSION_IMPLICATIONS,
        {
          $1: (urlString) => <b>{urlString}</b>,
          $2: (titleString) => <b>{titleString}</b>,
        },
      )
    // </if>
    default:
      return null
  }
}
