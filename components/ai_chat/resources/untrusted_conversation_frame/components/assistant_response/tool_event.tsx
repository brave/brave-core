// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import ToolEventContentUserChoice from './tool_event_content_user_choice'
import ToolEventContentAssistantDetailStorage from './tool_event_assistant_detail_storage'
import ToolEventTodos from './tool_event_todos'
import styles from './tool_event.module.scss'

interface Props {
  toolUseEvent: Mojom.ToolUseEvent
  isEntryActive: boolean
}

// Content customizable for each known tool type
export interface ToolUseContent {
  // Label to display for the action being taken. If not present,
  // then expandedContent is shown instead.
  toolLabel: string | null

  // UI for the tool action. If toolLabel is present, then this
  // will show on click. Otherwise, expandedContent will show by default.
  expandedContent: JSX.Element | null
}

export type ToolComponent = React.FC<
  Props & {
    // The params could be anything - it is created
    // by parsing LLM input so properties should be defensively
    // checked.
    toolInput: any

    // The component should pass data about the rendering of the tool use event
    // to the tool use event template component.
    children: (content: Partial<ToolUseContent>) => JSX.Element
  }
>

function ToolEventContent(
  props: Props & {
    children: (content: Partial<ToolUseContent>) => JSX.Element
  },
) {
  const { toolUseEvent } = props

  // defaults
  let content: ToolUseContent = {
    toolLabel: toolUseEvent.toolName,
    expandedContent: null,
  }

  // parse input
  const input = React.useMemo(() => {
    if (!toolUseEvent?.argumentsJson) {
      return null
    }
    try {
      return JSON.parse(toolUseEvent.argumentsJson)
    } catch (e) {
      return null
    }
  }, [toolUseEvent?.argumentsJson])

  // With no (or bad) input, don't render anything
  // for past entries. If this is an active entry,
  // we might be progressively rendering, so we can display some
  // in-progress UI.
  if (!input && !props.isEntryActive) {
    return null
  }

  // If we have a tool-specific component, use it, otherwise use the default
  let component: ToolComponent | null = null

  if (toolUseEvent.toolName === Mojom.USER_CHOICE_TOOL_NAME) {
    component = ToolEventContentUserChoice
  }

  if (toolUseEvent.toolName === Mojom.TODO_TOOL_NAME) {
    component = ToolEventTodos
  }

  if (toolUseEvent.toolName === Mojom.ASSISTANT_DETAIL_STORAGE_TOOL_NAME) {
    component = ToolEventContentAssistantDetailStorage
  }

  if (component) {
    return component({ ...props, toolInput: input })
  }

  // default
  return props.children(content)
}

export default function ToolEvent(props: Props) {
  const [isExpanded, setIsExpanded] = React.useState<boolean>(false)
  return (
    <ToolEventContent {...props}>
      {({ toolLabel, expandedContent }) => (
        <div
          className={classnames(
            styles.toolUse,
            props.isEntryActive && styles.isActive,
            toolLabel && expandedContent && styles.isExpandable,
            `tool-${props.toolUseEvent.toolName}`,
          )}
          onClick={() => {
            setIsExpanded(!isExpanded)
            return !expandedContent
          }}
        >
          {toolLabel && <div className={styles.toolText}>{toolLabel}</div>}

          {(!toolLabel || (isExpanded && expandedContent)) && (
            <div className={styles.expandedContent}>{expandedContent}</div>
          )}
        </div>
      )}
    </ToolEventContent>
  )
}
