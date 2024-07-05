/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'

export default function ConversationSkeleton() {
  return (
    <div className={styles.skeleton}>
      <div className={styles.turn}>
        <div
          className={styles.line}
          style={{ width: 50 }}
        />
        <div
          className={styles.line}
          style={{ width: 300 }}
        />
      </div>
      <div className={styles.turn}>
        <div
          className={styles.line}
          style={{ width: 80 }}
        />
        <div
          className={styles.line}
          style={{ width: 500 }}
        />
        <div
          className={styles.line}
          style={{ width: 450 }}
        />
        <div
          className={styles.line}
          style={{ width: 400 }}
        />
      </div>
    </div>
  )
}
