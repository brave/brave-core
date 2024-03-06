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

export default function WarningLongPage() {
  const context = React.useContext(DataContext)

  let warningText:any[]|string = getLocale('pageContentTooLongWarning')

  if (!context.isPremiumUser) {
    warningText = formatMessage(getLocale('pageContentTooLongWarningPremium'), {
      placeholders: {
        $1: context.siteInfo.truncatedContentPercentage &&
        context.siteInfo.truncatedContentPercentage.toFixed(2) + '%'
      }
    })
  }

  return (
    <div className={styles.info}>
      <div className={styles.infoIcon}>
        <Icon name='info-outline' />
      </div>
      <div className={styles.infoText}>{warningText}</div>
    </div>
  )
}
