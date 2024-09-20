// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './alerts.module.scss'

export default function LongConversationInfo() {
  const context = useConversation()
  const aiChatContext = useAIChat()

  const handleClearChat = () => {
    aiChatContext.onNewConversation()
    context.dismissLongConversationInfo()
  }

  return (
    <div className={styles.info}>
      <div className={styles.infoIcon}>
        <Icon name='info-outline' />
      </div>
      <div className={styles.infoText}>
        {getLocale('errorContextLimitReaching')}
        <Button kind="plain-faint" className={styles.link} onClick={handleClearChat}>
          <span>{getLocale('clearChatButtonLabel')}</span>
        </Button>
      </div>
    </div>
  )
}
