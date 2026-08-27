/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import WebsiteToolsModal from '../website_tools_modal'
import styles from './tools_attachments.module.scss'

interface Props {
  toolsContent: Mojom.AssociatedContent[]
  setToolsAttached: (
    content: Mojom.AssociatedContent,
    toolsAttached: boolean,
  ) => void
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
          <div
            key={content.uuid}
            className={styles.toolPill}
          >
            <Tooltip
              mode='mini'
              className={styles.toolPillTitleTooltip}
            >
              <button
                type='button'
                className={styles.toolPillOpen}
                onClick={() => setOpenContentUuid(content.uuid)}
              >
                <img
                  className={styles.toolPillFavicon}
                  src={`//favicon2?size=64&pageUrl=${encodeURIComponent(content.url.url)}&allowGoogleServerFallback=0`}
                />
                <span className={styles.toolPillTitle}>{content.title}</span>
              </button>
              <div slot='content'>{content.title}</div>
            </Tooltip>
            <Button
              fab
              size='tiny'
              kind='plain-faint'
              className={styles.toolPillRemove}
              title={getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_REMOVE_LABEL)}
              onClick={() => props.setToolsAttached(content, false)}
            >
              <Icon name='close' />
            </Button>
          </div>
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
