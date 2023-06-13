/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'

interface ConversationTileProps {
  title: string,
  date: string,
  isSelected: boolean
}

function ConversationTile(props: ConversationTileProps) {
  const contentStyles = classnames({
    [styles.content]: true,
    [styles.contentSelected]: props.isSelected
  })

  return (
    <div className={styles.box}>
      <div className={contentStyles}>
        <div className={styles.title}>{props.title}</div>
        <div className={styles.date}>{props.date}</div>
      </div>
    </div>
  )
}

export default ConversationTile
