// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { StyledWrapper, Alert, Icon } from './style'
import { Row } from '../../shared/style'

export interface Props {
  onClick: () => void
  onDismiss: () => void
  bannerType: 'warning' | 'error'
  description: string
  buttonText: string
}

export const WalletBanner = (props: Props) => {
  const { onDismiss, onClick, bannerType, description, buttonText } = props

  return (
    <StyledWrapper>
      <Alert
        type={bannerType}
      >
        <Icon
          slot='icon'
          name={
            bannerType === 'warning'
              ? 'warning-triangle-filled'
              : 'warning-circle-filled'
          }
        />
        {description}
        <Row
          alignItems='flex-start'
          slot='actions'
          width='unset'
        >
          <Button
            kind='plain'
            onClick={onClick}
            size='tiny'
          >
            {buttonText}
          </Button>
          <Button
            kind='plain-faint'
            onClick={onDismiss}
            size='tiny'
          >
            {getLocale('braveWalletDismissButton')}
          </Button>
        </Row>
      </Alert>
    </StyledWrapper>
  )
}

export default WalletBanner
