/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import ProgressRing from '@brave/leo/react/progressRing'
import classnames from '$web-common/classnames'
import { formatLocale, getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useConversation } from '../../state/conversation_context'
import { AttachmentPageItem } from '../attachment_item'
import styles from './style.module.scss'

interface Props {
  /** The attached content whose tools should be listed. */
  content: Mojom.AssociatedContent
  onClose: () => void
}

function ToolItem(props: {
  tool: Mojom.ToolInfo
  isExpanded: boolean
  onToggle: () => void
}) {
  const [isClamped, setIsClamped] = React.useState(false)
  const descriptionRef = React.useRef<HTMLSpanElement>(null)

  // Descriptions are only expandable when they're actually truncated. Keep the
  // last measurement while expanded - there's nothing to measure then.
  React.useLayoutEffect(() => {
    const element = descriptionRef.current
    if (!element || props.isExpanded) {
      return
    }
    const measure = () =>
      setIsClamped(element.scrollHeight > element.clientHeight)
    measure()
    const observer = new ResizeObserver(measure)
    observer.observe(element)
    return () => observer.disconnect()
  }, [props.tool.description, props.isExpanded])

  return (
    <li className={styles.tool}>
      <button
        type='button'
        disabled={!isClamped}
        aria-expanded={isClamped ? props.isExpanded : undefined}
        className={styles.toolToggle}
        onClick={props.onToggle}
      >
        <span className={styles.toolName}>{props.tool.name}</span>
        {/* The clamp needs its own element: a button lays its content out in
            an anonymous box, which -webkit-box doesn't survive. */}
        <span
          ref={descriptionRef}
          className={classnames({
            [styles.toolDescription]: true,
            [styles.toolDescriptionExpanded]: props.isExpanded,
          })}
        >
          {props.tool.description}
        </span>
      </button>
    </li>
  )
}

// Lists the tools an attached website exposes to Leo. Opened from the tools
// pill above the input box.
export default function WebsiteToolsModal(props: Props) {
  const conversation = useConversation()
  // Placeholder data means the page hasn't answered yet.
  const { getContentToolsData: tools, isPlaceholderData: isLoading } =
    conversation.api.useGetContentTools(props.content.uuid)
  // Only one description is expanded at a time, to keep the list scannable.
  const [expandedToolName, setExpandedToolName] = React.useState<string | null>(
    null,
  )

  return (
    <Dialog
      isOpen
      showClose
      onClose={props.onClose}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.title}
      >
        {getLocale(S.CHAT_UI_WEBSITE_TOOLS_TITLE)}
      </div>
      {/* In the header rather than the body so it stays pinned - the dialog
          scrolls once the tool list outgrows the viewport. */}
      <div
        slot='subtitle'
        className={styles.site}
      >
        <AttachmentPageItem
          title={props.content.title}
          url={props.content.url.url}
        />
      </div>
      <div className={styles.body}>
        {isLoading ? (
          <div className={styles.placeholder}>
            <ProgressRing />
          </div>
        ) : (
          <div className={styles.tools}>
            <div className={styles.toolsHeader}>
              {formatLocale(S.CHAT_UI_WEBSITE_TOOLS_LIST_LABEL, {
                $1: tools.length.toString(),
              })}
            </div>
            <ul className={styles.toolsList}>
              {tools.map((tool) => (
                <ToolItem
                  key={tool.name}
                  tool={tool}
                  isExpanded={tool.name === expandedToolName}
                  onToggle={() =>
                    setExpandedToolName((expanded) =>
                      expanded === tool.name ? null : tool.name,
                    )
                  }
                />
              ))}
            </ul>
          </div>
        )}
      </div>
    </Dialog>
  )
}
