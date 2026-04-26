// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'
import { getToolLabel } from '../assistant_response/get_tool_label'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import { isAssistantGroupTask } from '../conversation_entries/conversation_entries_utils'

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

export function useProgressBubbleContext () {
  return React.useContext(Context)
}

/**
 * Aids tracking when a sticky item is stuck, given lack of CSS support to
 * target an element only when sticky is active.
 * @param beforeRef A sentinel element positioned at the top sticky position
 * @param afterRef A sentinel element positioned at the bottom sticky position
 * @returns whether either sentinel is outside the area of intersection
 */
function useStickyState(
  beforeRef: React.RefObject<HTMLElement>,
  afterRef: React.RefObject<HTMLElement>,
) {
  const [isStuck, setIsStuck] = React.useState(false)

  React.useEffect(() => {
    const beforeEl = beforeRef.current
    const afterEl = afterRef.current
    if (!beforeEl || !afterEl) return

    const observer = new IntersectionObserver(
      (entries) => {
        let shouldBeInStuckState = false
        entries.forEach((entry) => {
          if (entry.target === beforeEl)
            shouldBeInStuckState = !entry.isIntersecting
          if (!shouldBeInStuckState && entry.target === afterEl)
            shouldBeInStuckState = !entry.isIntersecting
        })
        setIsStuck(shouldBeInStuckState)
      },
      { threshold: [0] },
    )

    observer.observe(beforeEl)
    observer.observe(afterEl)
    return () => observer.disconnect()
  }, [beforeRef, afterRef])

  return isStuck
}

export default function ProgressBubble(props: Props) {
  const conversationContext = useUntrustedConversationContext()
  const context = useProgressBubbleContext()

  const beforeSentinel = React.useRef<HTMLDivElement>(null)
  const afterSentinel = React.useRef<HTMLDivElement>(null)
  const isStuck = useStickyState(beforeSentinel, afterSentinel)

  const isGenerating = conversationContext.api.useStateData().isGenerating

  const isExpandable = props.responseGroup && isAssistantGroupTask(props.responseGroup)

  let progressText: string | null = null
  let isComplete = false

  if (isGenerating && props.isLastGroup) {
    progressText = "Thinking"

    // Get current "event"
    const lastEvent = props.responseGroup?.at(-1)?.events?.at(-1)
    if (lastEvent?.searchStatusEvent) {
      progressText = "Searching"
    }
    if (lastEvent?.toolUseEvent) {
      progressText = getToolLabel(lastEvent.toolUseEvent.toolName, null)
    }
    if (lastEvent?.deepResearchEvent) {
      // TODO
    }
  }

  if (!props.isLastGroup && isExpandable) {
    progressText = "Task complete"
    isComplete = true
  }

  if (!progressText) {
    return null
  }

  return (
    <>
      <div
        ref={beforeSentinel}
        className={classnames(styles.sentinel, styles.top)}
        aria-hidden='true'
      />
      <div
        className={classnames(
          styles.bubble,
          isStuck && styles.isStuck,
          !isComplete && styles.isActive,
          isExpandable && styles.isExpandable,
        )}
        onClick={
          isExpandable
            ? () => context.setIsExpanded(!context.isExpanded)
            : undefined
        }
      >
        {isComplete && (
          <Icon
            className={styles.icon}
            name='check-circle-outline'
          />
        )}
        {!isComplete && <ProgressRing className={styles.icon} />}
        <span className={styles.description}>{progressText}</span>
        {isExpandable && (
          <Icon
            className={styles.expander}
            name={context.isExpanded ? 'carat-up' : 'carat-right'}
          />
        )}
      </div>
      <div
        ref={afterSentinel}
        className={classnames(styles.sentinel, styles.bottom)}
        aria-hidden='true'
      />
    </>
  )
}
