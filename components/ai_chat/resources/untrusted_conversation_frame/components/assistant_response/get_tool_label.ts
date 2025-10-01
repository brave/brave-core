// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { formatLocale, getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'

function getNavigateToolNameLabel(toolInput: any) {
  if (!toolInput?.website_url) {
    return getLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE)
  }
  if (typeof toolInput.website_url === 'string') {
    return formatLocale(S.CHAT_UI_TOOL_LABEL_NAVIGATE_WEB_PAGE_WITH_INPUT, {
      $1: toolInput.website_url as string,
    })
  }
  return null
}

export function getToolLabel(toolName: string, toolInput: any) {
  // TODO(https://github.com/brave/brave-browser/issues/50097): add all other
  // known tool names as mojom constants and strings.
  switch (toolName) {
    case Mojom.NAVIGATE_TOOL_NAME:
      return getNavigateToolNameLabel(toolInput)
    default:
      return toolName
  }
}
