// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { Row } from '../../shared/style'

interface Props {
  onClickAdvancedSettings?: () => void
  onClickDetails?: () => void
}

export function ConfirmationFooterActions(props: Props) {
  const { onClickAdvancedSettings, onClickDetails } = props

  return (
    <Row justifyContent='space-between'>
      <div>
        {onClickAdvancedSettings && (
          <Button
            kind='plain'
            size='tiny'
            onClick={onClickAdvancedSettings}
          >
            <Icon
              name='settings'
              slot='icon-before'
            />
            {getLocale('braveWalletAdvancedTransactionSettings')}
          </Button>
        )}
      </div>
      <div>
        {onClickDetails && (
          <Button
            kind='plain'
            size='tiny'
            onClick={onClickDetails}
          >
            <Icon
              name='info-outline'
              slot='icon-before'
            />
            {getLocale('braveWalletDetails')}
          </Button>
        )}
      </div>
    </Row>
  )
}
