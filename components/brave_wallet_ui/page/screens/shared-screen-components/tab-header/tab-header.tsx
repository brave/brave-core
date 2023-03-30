// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Hooks
import { useOnClickOutside } from '../../../../common/hooks/useOnClickOutside'

// Types
import {
  WalletRoutes
} from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Styled Components
import {
  HeaderWrapper,
  BraveLogo,
  SettingsButton,
  SettingsIcon,
  SettingsWrapper
} from './tab-header.style'
import { HorizontalDivider, Row, Text } from '../../send/shared.styles'

// Components
import {
  WalletMorePopup
} from '../../../../components/desktop/wallet-more-popup/index'

interface Props {
  title: string
}

export const TabHeader = (props: Props) => {
  const { title } = props

  // Routing
  const history = useHistory()

  // State
  const [showSettings, setShowSettings] = React.useState<boolean>(false)

  // Refs
  const settingsModalRef = React.useRef<HTMLDivElement>(null)

  // Methods
  const onShowBackup = () => {
    history.push(WalletRoutes.Backup)
  }

  // Hooks
  useOnClickOutside(
    settingsModalRef,
    () => setShowSettings(false),
    showSettings
  )

  // render
  return (
    <HeaderWrapper>
      <Row rowHeight='full' verticalAlign='center'>
        <BraveLogo />
        <HorizontalDivider height={22} marginRight={12} />
        <Text textSize='18px' textColor='text02' isBold={true}>
          {getLocale(title)}
        </Text>
      </Row>
      <SettingsWrapper
        ref={settingsModalRef}
      >
        <SettingsButton onClick={() => setShowSettings(prev => !prev)}>
          <SettingsIcon name='more-horizontal' />
        </SettingsButton>
        {showSettings &&
          <WalletMorePopup
            onClickBackup={onShowBackup}
            onClosePopup={() => setShowSettings(false)}
            yPosition={50}
          />
        }
      </SettingsWrapper>
    </HeaderWrapper>
  )
}

export default TabHeader
