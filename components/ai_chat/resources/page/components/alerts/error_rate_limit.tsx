/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import DataContext from '../../state/context'
import PremiumSuggestion from '../premium_suggestion'
import styles from './alerts.module.scss'

interface ErrorRateLimit {
  onRetry?: () => void
}

function ErrorRateLimit(props: ErrorRateLimit) {
  const { isPremiumUser } = React.useContext(DataContext)

  if (!isPremiumUser) {
    return (
      <PremiumSuggestion
        title={getLocale('rateLimitReachedTitle')}
        description={getLocale('rateLimitReachedDesc')}
        secondaryActionButton={
          <Button kind='plain-faint' onClick={props.onRetry}>
            {getLocale('retryButtonLabel')}
          </Button>
        }
      />
    )
  }

  return (
    <div className={styles.alert}>
      <Alert
        mode='full'
        type='warning'
      >
        {getLocale('errorRateLimit')}
        <Button
          slot='actions'
          kind='filled'
          onClick={props.onRetry}
        >
            {getLocale('retryButtonLabel')}
        </Button>
      </Alert>
    </div>
  )
}

export default ErrorRateLimit
