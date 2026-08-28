/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import Dropdown from '@brave/leo/react/dropdown'
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

// Leo's Dropdown deals in strings, so the enum is stringified on the way in
// and parsed back out.
const PERMISSION_OPTIONS = [
  {
    permission: Mojom.ToolPermission.kAlwaysAllow,
    label: S.CHAT_UI_WEBSITE_TOOL_PERMISSION_ALWAYS_ALLOW,
  },
  {
    permission: Mojom.ToolPermission.kAsk,
    label: S.CHAT_UI_WEBSITE_TOOL_PERMISSION_ASK,
  },
  {
    permission: Mojom.ToolPermission.kNeverAllow,
    label: S.CHAT_UI_WEBSITE_TOOL_PERMISSION_NEVER_ALLOW,
  },
] as const

const DEFAULT_PERMISSION_OPTION = PERMISSION_OPTIONS.find(
  (option) => option.permission === Mojom.ToolPermission.kAsk,
)!

function ToolItem(props: {
  tool: Mojom.ToolInfo
  isExpanded: boolean
  onToggle: () => void
  onPermissionChange: (permission: Mojom.ToolPermission) => void
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

  const selected =
    PERMISSION_OPTIONS.find(
      (option) => option.permission === props.tool.permission,
    ) ?? DEFAULT_PERMISSION_OPTION

  return (
    <li className={styles.tool}>
      <div className={styles.toolHeader}>
        <span className={styles.toolName}>{props.tool.name}</span>
        <Dropdown
          size='small'
          className={styles.toolPermission}
          value={String(selected.permission)}
          // The dialog scrolls, which would clip an absolutely positioned menu.
          positionStrategy='fixed'
          onChange={(e: { value: string }) =>
            props.onPermissionChange(Number(e.value))
          }
        >
          <div slot='value'>{getLocale(selected.label)}</div>
          {PERMISSION_OPTIONS.map((option) => (
            <leo-option
              key={option.permission}
              value={String(option.permission)}
            >
              {getLocale(option.label)}
            </leo-option>
          ))}
        </Dropdown>
      </div>
      <button
        type='button'
        disabled={!isClamped}
        aria-expanded={isClamped ? props.isExpanded : undefined}
        className={styles.toolToggle}
        onClick={props.onToggle}
      >
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
  const { setContentToolPermission } =
    conversation.api.useSetContentToolPermission()
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
                  onPermissionChange={(permission) =>
                    setContentToolPermission([
                      props.content.uuid,
                      tool.name,
                      permission,
                    ])
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
