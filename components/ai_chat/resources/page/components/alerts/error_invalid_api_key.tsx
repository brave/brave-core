/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './alerts.module.scss'

interface ElementProps {
  onRetry?: () => void
}

export default function ErrorInvalidAPIKey(props: ElementProps) {
  const aiChatContext = useAIChat()

  return (
    <div className={styles.alert}>
      <Alert type='warning'>
        {getLocale(S.CHAT_UI_ERROR_INVALID_API_KEY)}
        <Button
          slot='actions'
          kind='filled'
          onClick={aiChatContext.api.actions.uiHandler.openAIChatSettings}
        >
          {getLocale(S.CHAT_UI_MODIFY_CONFIGURATION_LABEL)}
        </Button>
        <Button
          slot='actions'
          kind='filled'
          onClick={props.onRetry}
        >
          {getLocale(S.CHAT_UI_RETRY_BUTTON_LABEL)}
        </Button>
      </Alert>
    </div>
  )
}
