// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import useMediaQuery from '$web-common/useMediaQuery'

// Assets
import HardwareGraphicLightSvg from './images/hardware_graphic_light.svg'
import HardwareGraphicDarkSvg from './images/hardware_graphic_dark.svg'

// Utils
import { getLocale, formatLocale } from '$web-common/locale'
import { WalletRoutes } from '../../../../constants/types'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'

// Styled Components
import {
  Bold,
  HardwareGraphic,
} from './onboarding_connect_hardware_wallet.style'
import { Button } from '../onboarding.style'
import { Column, Row, Text } from '../../../../components/shared/style'

const walletHardwareDescription = formatLocale(
  S.BRAVE_WALLET_CONNECT_HARDWARE_WALLET_DESCRIPTION,
  {
    $1: <Bold>{getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_LEDGER)}</Bold>,
    $2: <Bold>{getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_TREZOR)}</Bold>,
  },
)

export const OnboardingConnectHardwareWallet = () => {
  // Hooks
  const history = useHistory()
  const isDarkMode = useMediaQuery('(prefers-color-scheme: dark)')

  // Methods
  const onClickContinue = () => {
    history.push(WalletRoutes.OnboardingHardwareWalletCreatePassword)
  }

  return (
    <OnboardingContentLayout>
      <Column
        width='100%'
        gap='32px'
      >
        <Text
          variant='heading.display3'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_CONNECT_HARDWARE_WALLET)}
        </Text>
        <HardwareGraphic
          src={isDarkMode ? HardwareGraphicDarkSvg : HardwareGraphicLightSvg}
        />
        <Column gap='16px'>
          <Text
            variant='large.regular'
            textColor='primary'
          >
            {getLocale(S.BRAVE_WALLET_IMPORT_HARDWARE_WALLET_DESCRIPTION)}
          </Text>
          <Text
            variant='default.regular'
            textColor='secondary'
          >
            {walletHardwareDescription}
          </Text>
        </Column>
        <Row>
          <Button onClick={onClickContinue}>
            {getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)}
          </Button>
        </Row>
      </Column>
    </OnboardingContentLayout>
  )
}
