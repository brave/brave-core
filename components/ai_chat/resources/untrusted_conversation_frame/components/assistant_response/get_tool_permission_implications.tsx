// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// <if expr="enable_ai_chat_tab_management_tool">
import * as React from 'react'
import { formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
// </if>

export function getToolPermissionImplications(toolName: string) {
  switch (toolName) {
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
