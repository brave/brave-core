/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import Icon from '@brave/leo/react/icon'

function ErrorRateLimit () {
  return (
    <div className={styles.box}>
      <Icon name="warning-circle-filled" className={styles.icon} />
      <div>
        <p>You've reached the maximum number of questions for Leo. Please try again in a few hours.</p>
      </div>
    </div>
  )
}

export default ErrorRateLimit
