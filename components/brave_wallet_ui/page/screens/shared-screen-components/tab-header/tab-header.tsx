// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../common/locale'

// Styled Components
import { HeaderWrapper, BraveLogo } from './tab-header.style'
import { HorizontalDivider, Row, Text } from '../../send/shared.styles'

// Components
// import { ToggleThemeButton } from './toggle-theme-button/toggle-theme-button'

interface Props {
  title: string
}

export const TabHeader = (props: Props) => {
  const { title } = props

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
      {/* Disabling Theme Toggle until we can make it work correctly with brave-core */}
      {/* <ToggleThemeButton onClick={() => { }} /> */}
    </HeaderWrapper>
  )
}

export default TabHeader
