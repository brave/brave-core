// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import DataContext from '../../state/context'
import styles from './alerts.module.scss'
import getPageHandlerInstance from '../../api/page_handler'

export default function InfoLongConversation() {
  const context = React.useContext(DataContext)

  const handleClearChat = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
    context.dismissLongConversationInfo()
  }

  return (
    <div className={styles.alert}>
      <Alert
        mode='full'
        type='info'
      >
        {getLocale('errorContextLimitReaching')}
        <Button
          slot='actions'
          kind='plain-faint'
          onClick={handleClearChat}
        >
          {getLocale('clearChatButtonLabel')}
        </Button>
        <Button
          slot='actions'
          kind='plain-faint'
          onClick={context.dismissLongConversationInfo}
        >
          {getLocale('dismissButtonLabel')}
        </Button>
      </Alert>
    </div>
  )
}
