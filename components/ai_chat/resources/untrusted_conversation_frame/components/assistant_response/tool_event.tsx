// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import ToolPermissionChallenge from '../tool_permission_challenge/tool_permission_challenge'
import { getToolLabel } from './get_tool_label'
import ToolEventContentUserChoice from './tool_event_content_user_choice'
import ToolEventContentAssistantDetailStorage from './tool_event_content_assistant_detail_storage'
import styles from './tool_event.module.scss'

interface Props {
  toolUseEvent: Mojom.ToolUseEvent
  /**
   * Whether the event is contained within an entry that is currently generating
   * or executing. This informs whether the tool can be interacted with, and
   * what to do with partial input.
   */
  isEntryActive: boolean

  /**
   * Whether the tool use request is currently being executed. This is usually
   * a short-lived state.
   */
  isExecuting: boolean
}

/**
 * Content customizable for each known tool type. Some tools provide
 * only a label, which will get styled depending on active state, some provide
 * a more expansive content instead, and some may provide both whereby the label
 * will be a button which toggles the expanded content.
 */
export interface ToolUseContent {
  /**
   * Label to display for the action being taken. If not present,
   * then expandedContent is shown instead. If present, then it is shown
   * immediately and expandedContent, if present, is shown or hidden on click.
   */
  toolLabel: string | null

  /**
   * UI for the tool action. If toolLabel is present, then this
   * will show on click. Otherwise, expandedContent will show by default.
   */
  expandedContent: JSX.Element | null
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
    children: (content: ToolUseContent) => JSX.Element | null
  }
>

/**
 * Decides what structured ToolUseContent to provide for a given tool. Provides
 * common functionality for all tools.
 */
function ToolEventContent(
  props: Props & {
    children: (content: ToolUseContent) => JSX.Element | null
  },
) {
  const { toolUseEvent } = props

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

  // defaults
  let content: ToolUseContent = {
    toolLabel: getToolLabel(toolUseEvent.toolName, input),
    // Note: we're not providing anything for expanded content by default. We
    // could consider showing the tool input and output in a generic way,
    // especially in an advanced mode. But this is mainly so that tools can
    // decide whether to show more detailed output, such as the the detailed
    // arguments to a non-critical tool.
    expandedContent: null,
  }

  // Halt for permission challenge
  if (toolUseEvent.permissionChallenge) {
    content.expandedContent = (
      <ToolPermissionChallenge
        isInteractive={props.isEntryActive}
        toolUseEvent={toolUseEvent}
        toolLabel={content.toolLabel!}
      />
    )
    content.toolLabel = null
    return props.children(content)
  }

  // Tool-specific components can add expanded content, a custom tool label,
  // or make the expanded content show by default by removing the toolLabel.
  let Component: ToolComponent | null = null

  if (toolUseEvent.toolName === Mojom.USER_CHOICE_TOOL_NAME) {
    Component = ToolEventContentUserChoice
  }

  if (toolUseEvent.toolName === Mojom.ASSISTANT_DETAIL_STORAGE_TOOL_NAME) {
    Component = ToolEventContentAssistantDetailStorage
  }

  if (Component) {
    return (
      <Component
        {...props}
        toolInput={input}
        content={content}
      />
    )
  }

  // default
  return props.children(content)
}

/**
 * Renders a tool event in a standardized way. Tools which don't want to display
 * in the regular timeline can opt-out of this by adding an exception to the
 * AssistantResponse component.
 */
export default function ToolEvent(props: Props) {
  return (
    <ToolEventContent {...props}>
      {(content) => {
        if (!content.toolLabel && !content.expandedContent) {
          // The tool content has decided to not show anything.
          // Perhaps it's an internal tool event, or a tool that's not ready
          // to render yet, given incomplete input.
          return null
        }

        const isExpandable = content.toolLabel && content.expandedContent

        // Show as executing if this is an active entry, we are currently
        // executing *a* tool, and this tool has no output yet.
        const isExecuting =
          props.isEntryActive && props.isExecuting && !props.toolUseEvent.output

        return (
          <div
            className={classnames(
              styles.toolUse,
              isExecuting && styles.isExecuting,
            )}
          >
            {isExpandable && (
              <ToolContentExpandable
                {...props}
                {...content}
              />
            )}

            {!isExpandable && content.toolLabel && (
              <div className={styles.toolLabel}>{content.toolLabel}</div>
            )}

            {!isExpandable && content.expandedContent && (
              <div>{content.expandedContent}</div>
            )}
          </div>
        )
      }}
    </ToolEventContent>
  )
}

function ToolContentExpandable(props: Props & ToolUseContent) {
  const [isExpanded, setIsExpanded] = React.useState<boolean>(false)

  return (
    <>
      <button
        className={classnames(styles.toolLabel, styles.isExpandable)}
        onClick={() => {
          setIsExpanded(!isExpanded)
        }}
      >
        {props.toolLabel}
      </button>
      {isExpanded && (
        <div className={styles.expandedContent}>{props.expandedContent}</div>
      )}
    </>
  )
}

export function ToolEventThinking() {
  return (
    <div
      className={classnames(styles.toolUse, styles.isExecuting)}
      data-testid='tool-event-thinking'
    >
      <div className={styles.toolLabel}>
        {getLocale(S.IDS_CHAT_UI_TOOL_LABEL_THINKING)}
      </div>
    </div>
  )
}
