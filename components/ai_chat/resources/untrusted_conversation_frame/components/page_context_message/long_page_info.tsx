// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import formatMessage from '$web-common/formatMessage'
import { getLocale } from '$web-common/locale'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'

export default function LongPageInfo() {
  const context = useUntrustedConversationContext()
  let warningText
  if (context.isContentRefined) {
    warningText = getLocale('pageContentRefinedWarning')
  } else if (context.trimmedTokens > 0 && context.totalTokens > 0) {
    const percentage =
      100
      - Math.floor(
        (Number(context.trimmedTokens) / Number(context.totalTokens)) * 100,
      )
    warningText = formatMessage(getLocale('trimmedTokensWarning'), {
      placeholders: {
        $1: percentage + '%',
      },
    })
  } else {
    warningText = formatMessage(getLocale('pageContentTooLongWarning'), {
      placeholders: {
        $1: context.contentUsedPercentage + '%',
      },
    })
  }

  return (
    <div className={styles.info}>
      <Icon name='info-outline' />
      <div>{warningText}</div>
    </div>
  )
}
