/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import styles from './tools_attachments.module.scss'

interface Props {
  toolsContent: Mojom.AssociatedContent[]
}

// A row of pills, one per attached content that provides tools, shown above the
// input. The "Tools" label stays pinned while the pills scroll horizontally.
export default function ToolsAttachments(props: Props) {
  if (props.toolsContent.length === 0) {
    return null
  }
  return (
    <div className={styles.toolsAttachments}>
      <div className={styles.toolsLabel}>
        <span>
          {getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_LABEL)} (
          {props.toolsContent.length})
        </span>
        <Tooltip mode='default'>
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
            <img
              className={styles.toolPillFavicon}
              src={`//favicon2?size=64&pageUrl=${encodeURIComponent(content.url.url)}&allowGoogleServerFallback=0`}
            />
            <Tooltip
              mode='mini'
              className={styles.toolPillTitleTooltip}
            >
              <span className={styles.toolPillTitle}>{content.title}</span>
              <div slot='content'>{content.title}</div>
            </Tooltip>
            <Button
              fab
              size='tiny'
              kind='plain-faint'
              className={styles.toolPillRemove}
              title={getLocale(S.CHAT_UI_TOOLS_ATTACHMENT_REMOVE_LABEL)}
            >
              <Icon name='close' />
            </Button>
          </div>
        ))}
      </div>
    </div>
  )
}
