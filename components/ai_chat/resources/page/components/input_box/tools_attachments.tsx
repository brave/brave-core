/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import { formatLocale, getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useConversation } from '../../state/conversation_context'
import WebsiteToolsModal from '../website_tools_modal'
import styles from './tools_attachments.module.scss'

interface Props {
  toolsContent: Mojom.AssociatedContent[]
  setToolsAttached: (
    content: Mojom.AssociatedContent,
    toolsAttached: boolean,
  ) => void
}

function ToolPill(props: {
  content: Mojom.AssociatedContent
  onOpen: () => void
  onRemove: () => void
}) {
  const conversation = useConversation()
  const { getContentToolsData: tools } = conversation.api.useGetContentTools(
    props.content.uuid,
  )

  return (
    <div className={styles.toolPill}>
      <Tooltip
        mode='mini'
        className={styles.toolPillTitleTooltip}
      >
        <button
          type='button'
          className={styles.toolPillOpen}
          onClick={props.onOpen}
        >
          <img
            className={styles.toolPillFavicon}
            src={`//favicon2?size=64&pageUrl=${encodeURIComponent(props.content.url.url)}&allowGoogleServerFallback=0`}
          />
          <span className={styles.toolPillTitle}>{props.content.title}</span>
          {tools.length > 0 && (
            <span
              className={styles.toolPillCount}
              role='img'
              aria-label={formatLocale(S.CHAT_UI_WEBSITE_TOOLS_LIST_LABEL, {
                $1: tools.length.toString(),
              })}
            >
              <Icon name='tool-box' />
              {tools.length}
            </span>
          )}
        </button>
        <div slot='content'>{props.content.title}</div>
      </Tooltip>
      <Button
        fab
        size='tiny'
        kind='plain-faint'
        className={styles.toolPillRemove}
        title={getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_REMOVE_LABEL)}
        onClick={props.onRemove}
      >
        <Icon name='close' />
      </Button>
    </div>
  )
}

// A row of pills, one per attached content that provides tools, shown above the
// input. The "Tools" label stays pinned while the pills scroll horizontally.
export default function ToolsAttachments(props: Props) {
  const [openContentUuid, setOpenContentUuid] = React.useState<string | null>(
    null,
  )

  if (props.toolsContent.length === 0) {
    return null
  }

  const openContent = props.toolsContent.find(
    (content) => content.uuid === openContentUuid,
  )

  return (
    <div className={styles.toolsAttachments}>
      <div className={styles.toolsLabel}>
        <span>{getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_LABEL)}</span>
        <Tooltip
          mode='default'
          positionStrategy='fixed'
        >
          <Icon name='info-outline' />
          <div
            slot='content'
            className={styles.toolsTooltipContent}
          >
            {getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_TOOLTIP_INFO)}
          </div>
        </Tooltip>
      </div>
      <div className={styles.toolsPills}>
        {props.toolsContent.map((content) => (
          <ToolPill
            key={content.uuid}
            content={content}
            onOpen={() => setOpenContentUuid(content.uuid)}
            onRemove={() => props.setToolsAttached(content, false)}
          />
        ))}
      </div>
      {/* Mounted only while open so the tool list is fetched fresh each time. */}
      {openContent && (
        <WebsiteToolsModal
          content={openContent}
          onClose={() => setOpenContentUuid(null)}
        />
      )}
    </div>
  )
}
