/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
// import { getLocale } from '$web-common/locale'

interface PromptAutoSuggestionProps {
  onRetry?: () => void
}

function ErrorConnection (props: PromptAutoSuggestionProps) {
  return (
    <div className={styles.box}>
      <Icon name="warning-circle-filled" className={styles.icon} />
      <div>
        <p>There was a network issue connecting to Leo, check your connection and try again.</p>
        <div className={styles.actionsBox}>
          <Button onClick={props.onRetry}>
            Retry
          </Button>
        </div>
      </div>
    </div>
  )
}

export default ErrorConnection
