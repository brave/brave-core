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
  PopupButton,
  MenuItemIcon,
  MenuItemRow,
  ButtonMenu,
  INTERACTIVE_ICON_COLOR,
  AddIcon,
  MenuButton
} from './wallet_menus.style'

export const AccountsMenu = () => {
  // routing
  const history = useHistory()

  // computed
  const isAndroid = loadTimeData.getBoolean('isAndroid') || false

  // render
  return (
    <ButtonMenu>
      <div slot='anchor-content'>
        <MenuButton
          kind='outline'
          padding='1px 0px'
          size='small'
        >
          <AddIcon
            size='18px'
            color={INTERACTIVE_ICON_COLOR}
          />
        </MenuButton>
      </div>
      {CreateAccountOptions.filter(
        (option) =>
          // Filter out hardware wallet item on Android.
          !isAndroid || option.name !== 'braveWalletConnectHardwareWallet'
      ).map((option) => (
        <PopupButton
          key={option.name}
          onClick={() => history.push(option.route)}
          minWidth={240}
        >
          <MenuItemRow>
            <MenuItemIcon name={option.icon} />
            {getLocale(option.name)}
          </MenuItemRow>
        </PopupButton>
      ))}
    </ButtonMenu>
  )
}

export default AccountsMenu
