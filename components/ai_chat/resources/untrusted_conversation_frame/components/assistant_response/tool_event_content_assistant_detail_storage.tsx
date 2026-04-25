// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import type { ToolComponent, ToolUseContent } from './tool_event'

const ToolEventContentAssistantDetailStorage: ToolComponent = (props) => {
  const content: ToolUseContent = {
    toolLabel: getLocale(S.CHAT_UI_TOOL_LABEL_ASSISTANT_DETAIL_STORAGE),
    expandedContent: null,
  }

  const detailText = React.useMemo(() => {
    // If we have parsed input, use that
    if (props.toolInput?.information) {
      return props.toolInput.information
    }
    // Otherwise use the raw input string and strip the "{"information": "" part
    // from the start of the string so that we render in-progress arguments.
    if (props.toolUseEvent.argumentsJson) {
      return props.toolUseEvent.argumentsJson.substring(16)
    }
    return ''
  }, [props.toolInput?.information, props.toolUseEvent.argumentsJson])

  content.expandedContent = detailText

  return props.children(content)
}

export default ToolEventContentAssistantDetailStorage
