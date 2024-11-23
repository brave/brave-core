// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import VisibilityTimer from '$web-common/visibilityTimer'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './notices.module.scss'
const illustrationUrl = require('./conversation_storage.svg')

export default function NoticeConversationStorage() {
  const aiChatContext = useAIChat()

  const noticeElementRef = React.useRef<HTMLDivElement>(null)

  // Dismiss the notice for future loads after 4 seconds of being visible
  React.useEffect(() => {
    if (!noticeElementRef.current) {
      return
    }

    const visibilityTimer = new VisibilityTimer(
      aiChatContext.markStorageNoticeViewed,
      4000,
      noticeElementRef.current
    )

    visibilityTimer.startTracking()

    return () => {
      visibilityTimer.stopTracking()
    }
  }, [noticeElementRef.current])

  return (
    <div
      className={styles.notice}
      ref={noticeElementRef}
    >
      <div className={styles.illustration}>
        <img src={illustrationUrl} alt="illustration" />
      </div>
      <div className={styles.content}>
        <h4>{getLocale('noticeConversationHistoryTitle')}</h4>
        <p>{getLocale('noticeConversationHistoryBody')}</p>
        <p>
          <a
            href='#'
            target='_blank'
            onClick={() => aiChatContext.uiHandler?.openModelSupportUrl()}
          >
            {getLocale('learnMore')}
          </a>

        </p>
      </div>
      <Button
        className={styles.closeButton}
        kind='plain-faint'
        fab
        title={getLocale('closeNotice')}
        onClick={aiChatContext.dismissStorageNotice}
      >
        <Icon name='close' />
      </Button>
    </div>
  )
}
