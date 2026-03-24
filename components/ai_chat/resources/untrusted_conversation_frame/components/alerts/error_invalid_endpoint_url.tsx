/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './alerts.module.scss'

export default function ErrorInvalidEndpointURL() {
  const context = useUntrustedConversationContext()

  const handleConfigureClick = () => {
    context.uiHandler.openAIChatCustomizationSettings()
  }

  return (
    <div className={styles.alert}>
      <Alert type='error'>
        {getLocale(S.CUSTOM_MODEL_ENDPOINT_INVALID_ERROR)}
        <Button
          slot='actions'
          kind='filled'
          onClick={handleConfigureClick}
        >
          {getLocale(S.CHAT_UI_MODIFY_CONFIGURATION_LABEL)}
        </Button>
      </Alert>
    </div>
  )
}
