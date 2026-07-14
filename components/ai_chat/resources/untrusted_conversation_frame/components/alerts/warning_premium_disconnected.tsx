// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './alerts.module.scss'

export default function WarningPremiumDisconnected() {
  const context = useUntrustedConversationContext()
  const [dismissed, setDismissed] = React.useState(false)

  if (dismissed) {
    return null
  }

  return (
    <div className={styles.alert}>
      <Alert type='warning'>
        {getLocale(S.CHAT_UI_PREMIUM_REFRESH_WARNING_DESCRIPTION)}
        <Button
          slot='actions'
          kind='filled'
          onClick={() => context.uiHandler.refreshPremiumSession()}
        >
          {getLocale(S.CHAT_UI_PREMIUM_REFRESH_WARNING_ACTION)}
        </Button>
        <Button
          slot='actions'
          kind='plain-faint'
          onClick={() => setDismissed(true)}
        >
          {getLocale(S.CHAT_UI_DISMISS_BUTTON_LABEL)}
        </Button>
      </Alert>
    </div>
  )
}
