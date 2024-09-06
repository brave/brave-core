// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import LeoButton from '@brave/leo/react/button'

// Utils
import { getLocale } from '$web-common/locale'

// Styled Components
import { HeaderButton, HeaderIcon } from './common.style'
import { Row } from '../../../shared/style'

interface Props {
  onClose?: () => void
  onBack?: () => void
  showHelp?: boolean
}

export const PostConfirmationHeader = (props: Props) => {
  const { onClose, onBack, showHelp } = props

  return (
    <Row
      padding='16px'
      justifyContent={onBack || showHelp ? 'space-between' : 'flex-end'}
    >
      {showHelp && (
        <div>
          <LeoButton
            kind='outline'
            size='tiny'
          >
            <Icon
              slot='icon-before'
              name='help-outline'
            />
            {getLocale('braveWalletGetHelp')}
          </LeoButton>
        </div>
      )}
      {onBack && (
        <HeaderButton onClick={onBack}>
          <HeaderIcon name='arrow-left' />
        </HeaderButton>
      )}
      {onClose && (
        <HeaderButton onClick={onClose}>
          <HeaderIcon name='close' />
        </HeaderButton>
      )}
    </Row>
  )
}
