// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// utils
import { getLocale } from '../../../../../common/locale'

// style
import { FullWidth, Text } from '../../../shared/style'
import { AlertIcon, WarningAlertRow } from './tx_warning_banner.styles'

type Props = {
  retrySimulation?: (() => void) | (() => Promise<void>)
  /**
   * Defaults to Transaction preview failed message
   */
  messageLocale?: string
}

export function TxWarningBanner({ retrySimulation, messageLocale }: Props) {
  // render
  return (
    <FullWidth>
      <WarningAlertRow justifyContent={'flex-start'}>
        <AlertIcon />
        <Text textSize='12px'>
          {getLocale(messageLocale || 'braveWalletTransactionPreviewFailed')}
        </Text>
        {retrySimulation ? (
          <Button
            kind='plain'
            onClick={retrySimulation}
          >
            {getLocale('braveWalletButtonRetry')}
          </Button>
        ) : null}
      </WarningAlertRow>
    </FullWidth>
  )
}
