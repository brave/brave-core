// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { HeaderWrapper, BraveLogo } from './tab-header.style'
import { Row } from '../../send/shared.styles'

// Components
import {
  WalletSettingsMenu //
} from '../../../../components/desktop/wallet-menus/wallet_settings_menu'

export interface Props {
  hideHeaderMenu?: boolean
}

export const TabHeader = (props: Props) => {
  const { hideHeaderMenu } = props

  // render
  return (
    <HeaderWrapper>
      <Row
        rowHeight='full'
        verticalAlign='center'
      >
        <BraveLogo />
      </Row>
      {!hideHeaderMenu && <WalletSettingsMenu anchorButtonKind='filled' />}
    </HeaderWrapper>
  )
}

export default TabHeader
