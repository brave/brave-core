// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import { useConversationEntriesContext } from '../../conversation_entries_context'
import styles from './style.module.scss'

export default function LongPageInfo() {
  const context = useConversationEntriesContext()
  const warningText = // context.contentUsedPercentage
    // ? getLocale('pageContentRefinedWarning') :
    formatMessage(getLocale('pageContentTooLongWarning'), {
    placeholders: {
      $1: context.contentUsedPercentage + '%'
    }
  })

  return (
    <div className={styles.info}>
      <Icon name='info-outline' />
      <div>{warningText}</div>
    </div>
  )
}
