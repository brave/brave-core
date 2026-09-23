/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Label from '@brave/leo/react/label'
import { getLocale } from '$web-common/locale'
import styles from './near_label_style.module.scss'
import NearLogo from '../../assets/near_ai.svg'

export function NearIcon() {
  return (
    <img
      src={NearLogo}
      className={styles.nearIcon}
    />
  )
}

export function NearLabel() {
  return (
    <Label
      color='green'
      mode='default'
    >
      <div
        slot='default'
        className={styles.labelContent}
      >
        <img
          src={NearLogo}
          className={styles.labelIcon}
        />
        {getLocale(S.AI_CHAT_VERIFIED_LABEL)}
      </div>
    </Label>
  )
}
