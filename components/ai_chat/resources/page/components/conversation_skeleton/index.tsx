/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'

const w50 = { width: 50 }
const w80 = { width: 80 }
const w300 = { width: 300 }
const w400 = { width: 400 }
const w450 = { width: 450 }
const w500 = { width: 500 }

export default function ConversationSkeleton() {
  return (
    <div className={styles.skeleton}>
      <div className={styles.turn}>
        <div
          className={styles.line}
          style={w50}
        />
        <div
          className={styles.line}
          style={w300}
        />
      </div>
      <div className={styles.turn}>
        <div
          className={styles.line}
          style={w80}
        />
        <div
          className={styles.line}
          style={w500}
        />
        <div
          className={styles.line}
          style={w450}
        />
        <div
          className={styles.line}
          style={w400}
        />
      </div>
    </div>
  )
}
