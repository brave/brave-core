// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import { getToolLabel } from '../assistant_response/get_tool_label'
import { isAssistantGroupTask } from '../conversation_entries/conversation_entries_utils'
import styles from './style.module.scss'

const INTERRUPTED_TASK_STATES = [
  Mojom.TaskState.kStopped,
  Mojom.TaskState.kPaused,
]

interface Props {
  responseGroup: Mojom.ConversationTurn[] | undefined
  isLastGroup: boolean
}

export interface ProgressBubbleContext {
  isExpanded: boolean
  setIsExpanded: (state: boolean) => void
}

const Context = React.createContext<ProgressBubbleContext>({
  isExpanded: false,
  setIsExpanded(state) {},
})

export function ProgressBubbleContextProvider(props: React.PropsWithChildren) {
  const [isExpanded, setIsExpanded] = React.useState<boolean>(false)

  return (
    <Context.Provider value={{ isExpanded, setIsExpanded }}>
      {props.children}
    </Context.Provider>
  )
}

export function useProgressBubbleContext() {
  return React.useContext(Context)
}

/**
 * Aids tracking when a sticky item is stuck, given lack of CSS support to
 * target an element only when sticky is active. Once CSS scroll-state
 * container queries are widely supported (Chrome shipped them in 133, but
 * WebKit support is still pending) this can be replaced with pure CSS.
 *
 * Returns callback refs to attach to two sentinel elements (the top sits at
 * the sticky `top` position, the bottom at the sticky `bottom` position) and
 * an `isStuck` flag that is true whenever either sentinel has scrolled out
 * of view. Callback refs are used so the observer attaches whenever the
 * sentinels mount — this matters when the consuming component renders them
 * conditionally, since a plain ref-object effect would otherwise fire once
 * on mount with `null` refs and never re-run.
 */
function useStickyState() {
  const [beforeEl, setBeforeEl] = React.useState<HTMLElement | null>(null)
  const [afterEl, setAfterEl] = React.useState<HTMLElement | null>(null)
  const [isStuck, setIsStuck] = React.useState(false)

  React.useEffect(() => {
    if (!beforeEl || !afterEl) return

    let topSentinelInView = false
    let bottomSentinelInView = false

    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          // The top sentinel sits just above the bubble's sticky `top`
          // position; it scrolls out of view exactly when the bubble starts
          // sticking.
          if (entry.target === beforeEl) {
            topSentinelInView = entry.isIntersecting
          }
          // The bottom sentinel sits just below the bubble's sticky `bottom`
          // position; when the bubble is pinned to the bottom this sentinel
          // is also offscreen — keep `isStuck` true so the styled state holds.
          if (entry.target === afterEl) {
            bottomSentinelInView = entry.isIntersecting
          }
        })
        setIsStuck(!topSentinelInView || !bottomSentinelInView)
      },
      { threshold: [0] },
    )

    observer.observe(beforeEl)
    observer.observe(afterEl)
    return () => observer.disconnect()
  }, [beforeEl, afterEl])

  return {
    isStuck,
    beforeRef: setBeforeEl,
    afterRef: setAfterEl,
    beforeEl,
  }
}

/**
 * Computes the bubble's display state (`progressText`, `isComplete`,
 * `isInterrupted`, `isExpandable`) from the response group and the current
 * conversation state. Pulls conversation state from context internally so
 * callers only have to supply what's specific to the bubble instance.
 */
function useProgressState(
  responseGroup: Mojom.ConversationTurn[] | undefined,
  isActiveGroup: boolean,
) {
  const conversationContext = useUntrustedConversationContext()
  const conversationState = conversationContext.api.useStateData()

  // We are in progress if we are generating, executing a tool, or waiting
  // for a tool.
  const groupIsWorking =
    isActiveGroup
    && (conversationState.isGenerating || conversationState.isToolExecuting)

  // Pick the most recent event that represents the bubble's status —
  // skip events that are extra data received in parallel, or that carry
  // their own dedicated UI rather than a status update.
  const lastEvent = responseGroup
    ?.at(-1)
    ?.events?.filter(
      (event) =>
        !event.conversationTitleEvent
        && !event.contentReceiptEvent
        && !event.inlineSearchEvent
        // Search status events are subsumed by the tool use event.
        && !event.searchQueriesEvent
        && !event.searchStatusEvent
        && !event.sourcesEvent,
    )
    .at(-1)

  const toolInput = React.useMemo(() => {
    if (lastEvent?.toolUseEvent?.argumentsJson) {
      try {
        return JSON.parse(lastEvent.toolUseEvent.argumentsJson)
      } catch (e) {
        return null
      }
    }
    return null
  }, [lastEvent?.toolUseEvent?.argumentsJson])

  // Since we only know the conversation's state, and not each entry's
  // state, we can only display interrupted states for the active entry.
  // See https://github.com/brave/brave-browser/issues/55283.
  const taskState = isActiveGroup
    ? conversationState.toolUseTaskState
    : Mojom.TaskState.kNone

  return React.useMemo(() => {
    const isExpandable = !!responseGroup && isAssistantGroupTask(responseGroup)

    let progressText: string | null = null
    let isComplete = false
    let isInterrupted = false

    if (groupIsWorking) {
      // Default to "Thinking"
      progressText = getLocale(S.CHAT_UI_TOOL_LABEL_THINKING)

      // While a tool is still in flight (no output yet), describe what it's
      // doing. Once the tool has produced output it's effectively done — fall
      // back to "Thinking" so the bubble reflects the model reasoning about
      // its next step.
      if (lastEvent?.toolUseEvent && !lastEvent.toolUseEvent.output) {
        const toolLabel = getToolLabel(
          lastEvent.toolUseEvent.toolName,
          toolInput,
        )
        if (toolLabel) {
          progressText = toolLabel
        }
      }
      // TODO(https://github.com/brave/brave-browser/issues/55016): handle
      // lastEvent?.deepResearchEvent and provide detailed status.

      // TODO(https://github.com/brave/brave-browser/issues/51418): provide
      // security scan event detail.
    } else if (isExpandable) {
      isComplete = true
      isInterrupted = INTERRUPTED_TASK_STATES.includes(taskState)
      if (taskState === Mojom.TaskState.kPaused) {
        progressText = getLocale(S.CHAT_UI_TASK_STATE_PAUSED_LABEL)
      }
      if (taskState === Mojom.TaskState.kStopped) {
        progressText = getLocale(S.CHAT_UI_TASK_STATE_STOPPED_LABEL)
      }

      if (!progressText) {
        progressText = getLocale(S.CHAT_UI_TOOL_LABEL_COMPLETE)
      }
    }

    return { progressText, isComplete, isInterrupted, isExpandable }
  }, [
    groupIsWorking,
    toolInput,
    lastEvent?.toolUseEvent,
    responseGroup,
    taskState,
  ])
}

export default function ProgressBubble(props: Props) {
  const context = useProgressBubbleContext()

  const { isStuck, beforeRef, afterRef, beforeEl } = useStickyState()

  const { progressText, isComplete, isInterrupted, isExpandable } =
    useProgressState(props.responseGroup, props.isLastGroup)

  // This component doesn't show if not expandable and not generating.
  if (!progressText) {
    return null
  }

  const onClickHandler = isExpandable
    ? () => {
        context.setIsExpanded(!context.isExpanded)
        // When expanding, make sure the start of the expanded content
        // is in view. This assumes that the ProgressBubble is directly above
        // the expanded/contracted content view. If that is not the case,
        // perhaps the scrollIntoView should be handled in the content's
        // component instead, reacting to context.isExpanded changing.
        beforeEl?.scrollIntoView(true)
      }
    : undefined

  return (
    <>
      <div
        ref={beforeRef}
        className={classnames(styles.sentinel, styles.top)}
        aria-hidden='true'
      />
      <div
        data-testid='progress-bubble'
        className={classnames(
          styles.bubble,
          isStuck && styles.isStuck,
          !isComplete && styles.isActive,
          isInterrupted && styles.isInterrupted,
          isExpandable && styles.isExpandable,
        )}
        onClick={onClickHandler}
      >
        {isComplete && (
          <Icon
            className={styles.icon}
            name={
              props.isLastGroup && isInterrupted
                ? 'close-circle'
                : 'check-circle-outline'
            }
          />
        )}
        {!isComplete && <ProgressRing className={styles.icon} />}
        <span
          data-testid='progress-bubble-description'
          className={styles.description}
        >
          {progressText}
        </span>
        {isExpandable && (
          <Icon
            className={styles.expander}
            name={context.isExpanded ? 'carat-up' : 'carat-right'}
          />
        )}
      </div>
      <div
        ref={afterRef}
        className={classnames(styles.sentinel, styles.bottom)}
        aria-hidden='true'
      />
    </>
  )
}
