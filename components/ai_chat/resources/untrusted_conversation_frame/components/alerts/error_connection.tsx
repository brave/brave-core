/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale, formatLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import styles from './alerts.module.scss'

interface Props {
  onRetry?: () => void
  errorDetails?: Mojom.APIErrorDetails | null
  isNearModel?: boolean
}

function ErrorConnection(props: Props) {
  const { errorDetails, isNearModel } = props

  let detailsText: string | undefined
  if (errorDetails) {
    if (errorDetails.errorType) {
      detailsText = formatLocale(S.CHAT_UI_ERROR_NETWORK_DETAILS, {
        $1: String(errorDetails.statusCode),
        $2: errorDetails.errorType,
      })
    } else if (errorDetails.innerStatusCode) {
      detailsText = formatLocale(S.CHAT_UI_ERROR_NETWORK_INNER_STATUS_CODE, {
        $1: String(errorDetails.statusCode),
        $2: String(errorDetails.innerStatusCode),
      })
    } else {
      detailsText = formatLocale(S.CHAT_UI_ERROR_NETWORK_STATUS_CODE, {
        $1: String(errorDetails.statusCode),
      })
    }
  }

  return (
    <div className={styles.alert}>
      <Alert type='error'>
        {getLocale(
          isNearModel ? S.CHAT_UI_ERROR_NETWORK_NEAR : S.CHAT_UI_ERROR_NETWORK,
        )}
        {detailsText && <p className={styles.errorDetails}>{detailsText}</p>}
        <Button
          slot='actions'
          kind='filled'
          onClick={props.onRetry}
        >
          {getLocale(S.CHAT_UI_RETRY_BUTTON_LABEL)}
        </Button>
      </Alert>
    </div>
  )
}

export default ErrorConnection
