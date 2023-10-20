/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import getPageHandlerInstance from '../../api/page_handler'
import styles from './style.module.scss'


function ErrorContextLimitReaching () {
  const handleClearChat = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  const handleDismiss = () => {}

  return (
    <div className={styles.box}>
      <Icon name="info-filled" className={styles.icon} />
      <div>
        <p>{getLocale('errorContextLimitReaching')}</p>
        <div className={styles.actionsBox}>
          <Button kind="plain" onClick={handleClearChat}>
            {getLocale('clearChatButtonLabel')}
          </Button>
          <Button kind="plain-faint" onClick={handleDismiss}>
            {getLocale('dismissButtonLabel')}
          </Button>
        </div>
      </div>
    </div>
  )
}

export default ErrorContextLimitReaching
