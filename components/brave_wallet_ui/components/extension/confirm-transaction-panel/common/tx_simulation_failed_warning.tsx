// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// utils
import { getLocale } from '../../../../../common/locale'

// style
import { Text } from '../../../shared/style'
import {
  AlertIcon,
  WarningAlertRow
} from './tx_simulation_failed_warning.styles'

type Props = {
  retrySimulation?: (() => void) | (() => Promise<void>)
}

export function TxSimulationFailedWarning({ retrySimulation }: Props) {
  // render
  return (
    <WarningAlertRow>
      <AlertIcon />
      <Text textSize='12px'>
        {getLocale('braveWalletTransactionPreviewFailed')}
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
  )
}
