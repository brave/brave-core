// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './notices.module.scss'
import illustrationUrl from './conversation_storage.svg'

export default function NoticeConversationStorage() {
  const aiChatContext = useAIChat()

  return (
    <div className={styles.notice}>
      <div className={styles.illustration}>
        <img
          src={illustrationUrl}
          alt='illustration'
        />
      </div>
      <div className={styles.content}>
        <h4>{getLocale(S.CHAT_UI_MENU_CONVERSATION_HISTORY)}</h4>
        <p>{getLocale(S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_BODY)}</p>
        <p>
          <button
            className={styles.learnMoreLink}
            onClick={() => aiChatContext.uiHandler?.openStorageSupportUrl()}
          >
            {getLocale(S.CHAT_UI_LEARN_MORE)}
          </button>
        </p>
      </div>
      <Button
        className={styles.closeButton}
        kind='filled'
        size='tiny'
        fab
        title={getLocale(S.CHAT_UI_CLOSE_NOTICE)}
        onClick={aiChatContext.dismissStorageNotice}
      >
        <Icon
          className={styles.closeIcon}
          name='close'
        />
      </Button>
    </div>
  )
}
