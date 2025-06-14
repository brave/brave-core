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

interface Props {
  _testIsCurrentModelLeo?: boolean
}

function ErrorRateLimit(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  // Respond to BYOM scenarios
  if (
    !conversationContext.isCurrentModelLeo ||
    props._testIsCurrentModelLeo === false
  ) {
    return (
      <div className={styles.alert}>
        <Alert type='warning'>
          {getLocale(S.CHAT_UI_ERROR_OAI_RATE_LIMIT)}
          <Button
            slot='actions'
            kind='filled'
            onClick={conversationContext.retryAPIRequest}
          >
            {getLocale(S.CHAT_UI_RETRY_BUTTON_LABEL)}
          </Button>
        </Alert>
      </div>
    )
  }

  // Respond to Leo (i.e., non-BYOM) scenarios
  if (!aiChatContext.isPremiumUser) {
    return (
      <PremiumSuggestion
        title={getLocale(S.CHAT_UI_RATE_LIMIT_REACHED_TITLE)}
        description={getLocale(S.CHAT_UI_RATE_LIMIT_REACHED_DESC)}
        secondaryActionButton={
          <Button
            kind='plain-faint'
            onClick={conversationContext.handleResetError}
          >
            {getLocale(S.AI_CHAT_MAYBE_LATER_LABEL)}
          </Button>
        }
      />
    )
  }

  return (
    <div className={styles.alert}>
      <Alert type='warning'>
        {getLocale(S.CHAT_UI_ERROR_RATE_LIMIT)}
        <Button
          slot='actions'
          kind='filled'
          onClick={conversationContext.retryAPIRequest}
        >
          {getLocale(S.CHAT_UI_RETRY_BUTTON_LABEL)}
        </Button>
      </Alert>
    </div>
  )
}

export default ErrorRateLimit
