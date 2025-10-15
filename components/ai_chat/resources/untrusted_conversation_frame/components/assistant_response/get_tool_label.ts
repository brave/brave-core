// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { formatLocale, getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'

/**
 * See navigation_tool.cc
 * @param toolInput Expects { "website_url": string } but is not guaranteed.
 * @returns
 */
function getNavigateToolNameLabel(toolInput: any) {
  if (typeof toolInput?.website_url === 'string') {
    return formatLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE_WITH_INPUT, {
      $1: toolInput.website_url as string,
    })
  }
  return getLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE)
}

/**
 * See history_tool.cc
 * @param toolInput Expects { "direction": "back" | "forward" } but is not guaranteed.
 * @returns
 */
function getHistoryToolNameLabel(toolInput: any) {
  // Tool input might not be complete, so check values explicitly.
  if (toolInput?.direction === 'back') {
    return getLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_HISTORY_BACK)
  } else if (toolInput?.direction === 'forward') {
    return getLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_HISTORY_FORWARD)
  }
  return getLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE)
}

/**
 * Get a display label for the specified tool given its name and input properties
 */
export function getToolLabel(toolName: string, toolInput: any) {
  switch (toolName) {
    case Mojom.NAVIGATE_TOOL_NAME:
      return getNavigateToolNameLabel(toolInput)
    case Mojom.NAVIGATE_HISTORY_TOOL_NAME:
      return getHistoryToolNameLabel(toolInput)
    case Mojom.CLICK_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_CLICK_ELEMENT)
    case Mojom.DRAG_AND_RELEASE_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_DRAG_AND_RELEASE)
    case Mojom.MOVE_MOUSE_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_MOVE_MOUSE)
    case Mojom.SCROLL_ELEMENT_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_SCROLL_ELEMENT)
    case Mojom.SELECT_DROPDOWN_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_SELECT_DROPDOWN)
    case Mojom.TYPE_TEXT_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_TYPE_TEXT)
    case Mojom.WAIT_TOOL_NAME:
      return getLocale(S.CHAT_UI_TOOL_LABEL_WAIT)
    default:
      return toolName
  }
}
