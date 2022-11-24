// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { HeaderWrapper, BraveLogo } from './header.style'
import { HorizontalDivider, Row, Text } from '../../shared.styles'

// Components
// import { ToggleThemeButton } from './toggle-theme-button/toggle-theme-button'

export const SendHeader = () => {
  // render
  return (
    <HeaderWrapper>
      <Row rowHeight='full' verticalAlign='center'>
        <BraveLogo />
        <HorizontalDivider height={22} marginRight={12} />
        <Text textSize='18px' textColor='text02' isBold={true}>
          {getLocale('braveWalletSend')}
        </Text>
      </Row>
      {/* Disabling Theme Toggle until we can make it work correctly with brave-core */}
      {/* <ToggleThemeButton onClick={() => { }} /> */}
    </HeaderWrapper>
  )
}

export default SendHeader
