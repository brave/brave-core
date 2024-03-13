/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import getPageHandlerInstance from '../../api/page_handler'
import DataContext from '../../state/context'
import styles from './alerts.module.scss'

function ErrorGeneratedQuestionsRateLimit() {
  const context = React.useContext(DataContext)

  return (
    <div className={styles.alert}>
      <Alert
        mode='full'
        type='error'
      >
        {context.isPremiumUser
          ? getLocale('premiumRateLimitErrorForGeneratedQuestionsLabel')
          : getLocale('rateLimitErrorForGeneratedQuestionsLabel')}
        <Button
          slot='actions'
          kind='filled'
          onClick={() =>
            context.isPremiumUser
              ? context.switchModel()
              : getPageHandlerInstance().pageHandler.goPremium()
          }
        >
          {context.isPremiumUser
            ? getLocale('switchModelLabel')
            : getLocale('goPremiumLabel')}
        </Button>
        <Button
          slot='actions'
          kind='plain-faint'
          onClick={() => {}}
        >
          {getLocale('dismissButtonLabel')}
        </Button>
      </Alert>
    </div>
  )
}

export default ErrorGeneratedQuestionsRateLimit
