// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale } from '$web-common/locale'

// Styled Components
import { Alert } from './common.style'

interface Props {
  onSpeedUp?: () => void
}

export const SpeedUpAlert = (props: Props) => {
  const { onSpeedUp } = props

  return (
    <Alert
      type='info'
      mode='simple'
    >
      {getLocale('braveWalletTransactionTakingLongTime')}
      <div slot='content-after'>
        <Button
          onClick={onSpeedUp}
          kind='outline'
          size='tiny'
        >
          {getLocale('braveWalletTransactionSpeedup')}
        </Button>
      </div>
    </Alert>
  )
}
