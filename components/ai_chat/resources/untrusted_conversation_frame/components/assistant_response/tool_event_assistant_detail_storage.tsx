// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import type { ToolComponent, ToolUseContent } from './tool_event'
import styles from './tool_event.module.scss'

const ToolEventContentAssistantDetailStorage: ToolComponent = (props) => {
  const content: ToolUseContent = {
    // TODO: translation
    toolLabel: 'Noting down some information for later about the page',
    expandedContent: null,
  }

  const detailText = React.useMemo(() => {
    // If we have parsed input, use that
    if (props.toolInput?.information) {
      return props.toolInput.information
    }
    // Otherwise use the raw input string and strip the "{"information": "" part
    // from the start of the string
    if (props.toolUseEvent.argumentsJson) {
      return props.toolUseEvent.argumentsJson.substring(16)
    }
    return ''
  }, [props.toolInput?.information, props.toolUseEvent.argumentsJson])


  content.expandedContent = (
    <div className={styles.assistantStorageDetail}>
      {detailText}
    </div>
  )

  return props.children(content)
}

export default ToolEventContentAssistantDetailStorage
