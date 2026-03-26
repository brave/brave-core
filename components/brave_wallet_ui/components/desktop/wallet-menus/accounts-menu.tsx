// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

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
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
} from './wellet-menus.style'

interface Props {
  hiddenAccounts: BraveWallet.AccountInfo[]
  onCloseMenu: () => void
}

export const AccountsMenu = ({ hiddenAccounts, onCloseMenu }: Props) => {
  // routing
  const history = useHistory()

  // Selectors
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  return (
    <StyledWrapper yPosition={42}>
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
        <PopupButton
          key={option.name}
          onClick={() => {
            history.push(option.route)
            onCloseMenu()
          }}
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
