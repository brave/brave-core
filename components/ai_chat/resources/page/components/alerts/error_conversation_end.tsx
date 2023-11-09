/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import getPageHandlerInstance from '../../api/page_handler'
import styles from './alerts.module.scss'

function ErrorConversationEnd () {
  return (
    <div className={styles.alert}>
      <Alert
        mode='full'
        type='error'
      >
        {getLocale('errorConversationEnd')}
        <Button
          slot='actions'
          kind='plain-faint'
          onClick={() => { getPageHandlerInstance().pageHandler.clearConversationHistory() }}
        >
            {getLocale('menuNewChat')}
        </Button>
      </Alert>
    </div>
  )
}

export default ErrorConversationEnd
