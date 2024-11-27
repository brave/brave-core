/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import PremiumSuggestion from '../premium_suggestion'
import styles from './alerts.module.scss'

function ErrorRateLimit() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  if (!aiChatContext.isPremiumUser) {
    return (
      <PremiumSuggestion
        title={getLocale('rateLimitReachedTitle')}
        description={getLocale('rateLimitReachedDesc')}
        secondaryActionButton={
          <Button kind='plain-faint' onClick={conversationContext.handleResetError}>
            {getLocale('maybeLaterLabel')}
          </Button>
        }
      />
    )
  }

  return (
    <div className={styles.alert}>
      <Alert
        type='warning'
      >
        {getLocale('errorRateLimit')}
        <Button
          slot='actions'
          kind='filled'
          onClick={conversationContext.retryAPIRequest}
        >
            {getLocale('retryButtonLabel')}
        </Button>
      </Alert>
    </div>
  )
}

export default ErrorRateLimit
