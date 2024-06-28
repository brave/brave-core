// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import DataContext from '../../state/context'
import styles from './alerts.module.scss'
import formatMessage from '$web-common/formatMessage'

export default function LongPageInfo() {
  const context = React.useContext(DataContext)


  const warningText = context.siteInfo.isContentRefined
    ? getLocale('pageContentRefinedWarning')
    : formatMessage(getLocale('pageContentTooLongWarning'), {
    placeholders: {
      $1: context.siteInfo.contentUsedPercentage + '%'
    }
  })

  return (
    <div className={styles.info}>
      <div className={styles.infoIcon}>
        <Icon name='info-outline' />
      </div>
      <div className={styles.infoText}>{warningText}</div>
    </div>
  )
}
