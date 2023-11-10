// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { loadTimeData } from '../../../../common/loadTimeData'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'

export const AccountsMenu = () => {
  // routing
  const history = useHistory()

  const isAndroid = loadTimeData.getBoolean('isAndroid') || false
  return (
    <StyledWrapper yPosition={42}>
      {CreateAccountOptions.filter(option => (
        // Filter out hardware wallet item on Android.
        !isAndroid || option.name !== 'braveWalletConnectHardwareWallet'
      )).map((option) => (
        <PopupButton
          key={option.name}
          onClick={() => history.push(option.route)}
          minWidth={240}
        >
          <ButtonIcon name={option.icon} />
          <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
        </PopupButton>
      ))}
    </StyledWrapper>
  )
}

export default AccountsMenu
