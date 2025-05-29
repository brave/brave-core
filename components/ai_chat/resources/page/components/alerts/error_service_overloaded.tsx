/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import styles from './alerts.module.scss'

interface ElementProps {
  onRetry?: () => void
}

export default function ErrorServiceOverloaded(props: ElementProps) {
  return (
    <div className={styles.alert}>
      <Alert type='error'>
        {getLocale(StringIds.ErrorServiceOverloaded)}
        <Button
          slot='actions'
          kind='filled'
          onClick={props.onRetry}
        >
          {getLocale(StringIds.RetryButtonLabel)}
        </Button>
      </Alert>
    </div>
  )
}
