// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { formatLocale } from '$web-common/locale'
import styles from './style.module.scss'

function LongPageInfoInternal({ warningText }: { warningText: string }) {
  return (
    <div className={styles.info}>
      <Icon name='info-outline' />
      <div>{warningText}</div>
    </div>
  )
}

export function LongVisualContentWarning({
  visualContentUsedPercentage,
}: {
  visualContentUsedPercentage: number
}) {
  return (
    <LongPageInfoInternal
      warningText={formatLocale(S.CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING, {
        $1: visualContentUsedPercentage + '%',
      })}
    />
  )
}

export function LongTextContentWarning({
  percentageUsed,
}: {
  percentageUsed: number
}) {
  return (
    <LongPageInfoInternal
      warningText={formatLocale(S.CHAT_UI_TRIMMED_TOKENS_WARNING, {
        $1: percentageUsed + '%',
      })}
    />
  )
}

export function LongPageContentWarning({
  contentUsedPercentage,
}: {
  contentUsedPercentage: number
}) {
  return (
    <LongPageInfoInternal
      warningText={formatLocale(S.CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING, {
        $1: contentUsedPercentage + '%',
      })}
    />
  )
}
