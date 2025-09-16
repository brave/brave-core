// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import type { ToolComponent } from './tool_event'
import styles from './tool_event.module.scss'

const ToolEventContentAssistantDetailStorage: ToolComponent = (props) => {
  const [isExpanded, setIsExpanded] = React.useState<boolean | null>(null)

  // Show expanded by default if the tool isn't completed yet
  const showExpanded =
    isExpanded !== null ? isExpanded : !props.toolUseEvent.output

  const content = props.content

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

  content.statusIcon = <Icon name='database' />

  content.progressIcon = <Icon name='database' />

  // TODO: translation
  content.toolText = (
    <div
      className={styles.assistantStorageDetailTrigger}
      onClick={() => setIsExpanded(!isExpanded)}
    >
      Noting down some information for later about the page
      {showExpanded && (
        <div className={styles.assistantStorageDetail}>{detailText}</div>
      )}
    </div>
  )

  return props.children(content)
}

export default ToolEventContentAssistantDetailStorage
