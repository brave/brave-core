// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { formatLocale, getLocale } from '$web-common/locale'
import type { Value } from 'gen/mojo/public/mojom/base/values.mojom.m.js'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import type { ToolComponent } from './tool_event'
import styles from './tool_event_tab_management.module.scss'

const ToolEventTabManagement: ToolComponent = (props) => {
  const context = useUntrustedConversationContext()

  const content = props.content

  const needsUserPermission =
    props.toolUseEvent.requiresUserInteraction && props.isEntryActive

  if (needsUserPermission) {
    const handleSetPermission = (granted: boolean) => {
      context.conversationHandler?.useToolWithClientData(
        props.toolUseEvent.id,
        { boolValue: granted } as Value,
      )
    }

    content.expandedContent = (
      <div>
        <label>
          {formatLocale(S.AI_CHAT_TOOL_TAB_MANAGEMENT_PERMISSION_DESCRIPTION, {
            $1: (urlString) => <b>{urlString}</b>,
            $2: (titleString) => <b>{titleString}</b>,
          })}
        </label>
        <div className={styles.actions}>
          <Button
            size='small'
            onClick={() => handleSetPermission(true)}
          >
            {getLocale(S.AI_CHAT_TOOL_TAB_MANAGEMENT_PERMISSION_ALLOW_BUTTON)}
          </Button>
          <Button
            size='small'
            onClick={() => handleSetPermission(false)}
          >
            {getLocale(S.AI_CHAT_TOOL_TAB_MANAGEMENT_PERMISSION_DENY_BUTTON)}
          </Button>
        </div>
      </div>
    )
  } else if (props.isEntryActive) {
    content.toolLabel = getLocale(S.AI_CHAT_TOOL_TAB_MANAGEMENT_ACTIVE_STATE_DESCRIPTION)
  } else {
    // TODO(https://github.com/brave/brave-browser/issues/48981): Alternate UI
    // state for user having previously denied permission. We could check output
    // (but that's ugly string-matching), or we could preserve the client data
    // in the tool use event...
    content.toolLabel = getLocale(S.AI_CHAT_TOOL_TAB_MANAGEMENT_COMPLETED_STATE_DESCRIPTION)
  }

  // if (props.toolUseEvent.output) {
  //   content.tooltipContent = (
  //     <div>
  //       {props.toolUseEvent.output.map((output, i) => (
  //         <div key={output.textContentBlock?.text || i}>
  //           {output.textContentBlock?.text}
  //         </div>
  //       ))}
  //     </div>
  //   )
  // }

  return props.children(content)
}

export default ToolEventTabManagement
