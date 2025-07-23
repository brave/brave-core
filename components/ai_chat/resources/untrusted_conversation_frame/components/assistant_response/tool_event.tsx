// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import ToolEventContentUserChoice from './tool_event_content_user_choice'
import styles from './tool_event.module.scss'

interface Props {
  toolUseEvent: Mojom.ToolUseEvent
  isEntryActive: boolean
}

// Content customizable for each known tool type
export interface ToolUseContent {
  toolText: JSX.Element
  tooltipContent: JSX.Element | null
  statusIcon: JSX.Element
  progressIcon: JSX.Element
}

export type ToolComponent = React.FC<
  Props & {
    // The params could be anything - it is created
    // by parsing LLM input so properties should be defensively
    // checked.
    toolInput: any
    // default content for the component to modify any property
    content: ToolUseContent
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
    toolText: <>{toolUseEvent.toolName}</>,
    tooltipContent: null,
    statusIcon: <span data-testid='tool-default-completed-icon'><Icon name='check-circle-outline' /></span>,
    progressIcon: <span data-testid='tool-default-progress-icon'><ProgressRing /></span>,
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

  if (component) {
    return component({ ...props, toolInput: input, content })
  }

  // default
  return props.children(content)
}

export default function ToolEvent(props: Props) {
  const isComplete = !!props.toolUseEvent.output
  return (
    <ToolEventContent {...props}>
      {({ progressIcon, statusIcon, toolText, tooltipContent }) => (
        <div
          className={classnames(
            styles.toolUse,
            isComplete && styles.toolUseComplete,
            `tool-${props.toolUseEvent.toolName}`,
          )}
        >
          <div
            className={styles.toolUseIcon}
            title={props.toolUseEvent.argumentsJson}
          >
            {isComplete
              ? statusIcon
              : progressIcon
            }
          </div>
          {tooltipContent ? (
            <Tooltip>
              {toolText}
              <div slot='content'>{tooltipContent}</div>
            </Tooltip>
          ) : (
            toolText
          )}
        </div>
      )}
    </ToolEventContent>
  )
}
