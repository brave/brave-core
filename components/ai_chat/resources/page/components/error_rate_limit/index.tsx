/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import DataContext from '../../state/context'
import styles from './style.module.scss'
import ErrorRateLimitPremium from '../error_rate_limit_premium'

interface ErrorRateLimit {
  onRetry?: () => void
}

function ErrorRateLimit(props: ErrorRateLimit) {
  const { isPremiumUser } = React.useContext(DataContext)

  return (
    <div className={styles.common}>
      {isPremiumUser ? (
        <div className={styles.box}>
          <Icon name='warning-triangle-filled' className={styles.icon} />
          <div>
            <p>{getLocale('errorRateLimit')}</p>
            <div className={styles.actionsBox}>
              <Button onClick={props.onRetry}>
                {getLocale('retryButtonLabel')}
              </Button>
            </div>
          </div>
        </div>
      ) : (
        <ErrorRateLimitPremium
          title={getLocale('rateLimitReachedTitle')}
          description={getLocale('rateLimitReachedDesc')}
          onRetry={props.onRetry}
        />
      )}
    </div>
  )
}

export default ErrorRateLimit
