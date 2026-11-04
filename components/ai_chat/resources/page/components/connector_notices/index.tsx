// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/nala/react/alert'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import {
  getConnectorNoticeText,
  isConnectorFailing,
} from '../connectors_modal/connector_utils'
import styles from './style.module.scss'

/**
 * A notice for each of the user's connectors that the latest answer was
 * written without, as it wasn't available. It's in the page rather than the
 * conversation frame, which isn't told about connectors.
 */
export default function ConnectorNotices() {
  const aiChat = useAIChat()
  const conversation = useConversation()

  // The notices are about an answer, so wait until it's written.
  const missed = conversation.isGenerating
    ? []
    : conversation.associatedContentInfo.filter(
        (content) =>
          content.mcpServer?.missedLatestAnswer
          && isConnectorFailing(content.mcpServer.status),
      )

  const closeLabel = getLocale(S.CHAT_UI_CLOSE_NOTICE)
  // Always there, so that a notice that appears in it is announced.
  return (
    <div
      role='status'
      className={styles.notices}
    >
      {missed.map((content) => (
        <Alert
          key={content.uuid}
          type='warning'
          size='small'
          data-testid='connector-notice'
        >
          {getConnectorNoticeText(content.mcpServer!.status, content.title)}
          <Button
            slot='actions'
            kind='outline'
            size='small'
            onClick={() => aiChat.setConnectorsDialog({ view: 'list' })}
          >
            {getLocale(S.CHAT_UI_CONNECTOR_NOTICE_MANAGE_BUTTON_LABEL)}
          </Button>
          <Button
            slot='content-after'
            fab
            kind='plain-faint'
            size='small'
            title={closeLabel}
            aria-label={closeLabel}
            onClick={() =>
              conversation.api.conversationHandler.dismissMCPServerNotice(
                content.uuid,
              )
            }
          >
            <Icon name='close' />
          </Button>
        </Alert>
      ))}
    </div>
  )
}
