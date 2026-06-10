// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Components
import { DefaultPanelHeader } from './default-panel-header'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

import { AccountsMenu } from '../wallet-menus/accounts-menu'

// Styled Components
import {
  HeaderTitle,
  MenuButton,
  MenuButtonIcon,
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

interface Props {
  hiddenAccounts: BraveWallet.AccountInfo[]
}

export const AccountsHeader = ({ hiddenAccounts }: Props) => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  return isPanel || isMobile ? (
    <DefaultPanelHeader
      title={getLocale('braveWalletTopNavAccounts')}
      expandRoute={WalletRoutes.Accounts}
    />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle
        textColor='primary'
        variant='heading.h1'
      >
        {getLocale('braveWalletTopNavAccounts')}
      </HeaderTitle>
      <AccountsMenu hiddenAccounts={hiddenAccounts}>
        <MenuButton slot='anchor-content'>
          <MenuButtonIcon name='plus-add' />
        </MenuButton>
      </AccountsMenu>
    </Row>
  )
}

export default AccountsHeader
