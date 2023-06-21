/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import Icon from '@brave/leo/react/icon'

interface MainProps {
  conversationList: React.ReactNode
  inputBox: React.ReactNode
  siteTitle: React.ReactNode
}

function Main (props: MainProps) {
  return (
    <main className={styles.main}>
      <div className={styles.header}>
        <div className={styles.logoBox}>
          <Icon name="product-brave-ai" />
          <div className={styles.logoTitle}>Brave <span>Leo</span></div>
        </div>
      </div>
      <div className={styles.scroller}>
        <div className={styles.siteTitleBox}>
          {props.siteTitle}
        </div>
        {props.conversationList}
      </div>
      <div className={styles.inputBox}>
        {props.inputBox}
      </div>
    </main>
  )
}

export default Main
