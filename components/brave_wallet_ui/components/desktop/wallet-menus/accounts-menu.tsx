// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Icon from '@brave/leo/react/icon'

// Selectors
import {
  useSafeUISelector, //
} from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

interface Props {
  children: React.ReactNode
  hiddenAccounts: BraveWallet.AccountInfo[]
}

export const AccountsMenu = ({ children, hiddenAccounts }: Props) => {
  // routing
  const history = useHistory()

  // Selectors
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  return (
    <ButtonMenu
      placement='bottom-end'
      minWidth={240}
    >
      {children}
      {CreateAccountOptions.filter((option) => {
        // Filter out hardware wallet item on Android.
        if (isMobile && option.name === 'braveWalletConnectHardwareWallet') {
          return false
        }
        // Show restore route only when we have hidden accounts.
        if (
          option.name === 'braveWalletWelcomeRestoreButton'
          && hiddenAccounts.length === 0
        ) {
          return false
        }
        return true
      }).map((option) => (
        <leo-menu-item
          key={option.name}
          onClick={() => history.push(option.route)}
        >
          <Icon name={option.icon} />
          {getLocale(option.name)}
        </leo-menu-item>
      ))}
    </ButtonMenu>
  )
}

export default AccountsMenu
