// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import styles from './alerts.module.scss'

export default function TemporaryChatInfo() {
  return (
    <div className={styles.alert}>
      <Alert type='info'>
        <div slot='title'>{getLocale('temporaryChatLabel')}</div>
        <Icon name='message-bubble-temporary' slot='icon' />
        {getLocale('temporaryChatInfo')}
      </Alert>
    </div>
  )
}
