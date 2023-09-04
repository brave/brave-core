// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Actions
import { PanelActions } from '../../../panel/actions'

// Queries
import { useSelectedAccountQuery } from '../../../common/slices/api.slice.extra'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'

// Components
import { DividerLine } from '../divider/index'
import { ConnectedAccountItem } from '../connected-account-item/index'
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'

// Styled Components
import {
  StyledWrapper,
  AddressContainer,
  AddressScrollContainer,
  AccountsTitle,
  SiteOriginTitle,
  HeaderRow,
  HeaderColumn,
  FavIcon,
  NewAccountButton
} from './style'

export const SitePermissions = () => {
  const dispatch = useDispatch()
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)
  const connectedAccounts = useUnsafeWalletSelector(
    WalletSelectors.connectedAccounts
  )

  // api
  const { data: selectedAccount } = useSelectedAccountQuery()
  const selectedCoin = selectedAccount?.accountId.coin

  // methods
  const onAddAccount = React.useCallback(() => {
    dispatch(PanelActions.expandWalletAccounts())
  }, [])

  // memos
  const accountByCoinType = React.useMemo((): BraveWallet.AccountInfo[] => {
    return accounts.filter((account) => account.accountId.coin === selectedCoin)
  }, [accounts, selectedCoin])

  return (
    <StyledWrapper>
      <HeaderRow>
        <FavIcon src={`chrome://favicon/size/64@1x/${activeOrigin.originSpec}`} />
        <HeaderColumn>
          <SiteOriginTitle>
            <CreateSiteOrigin
              originSpec={activeOrigin.originSpec}
              eTldPlusOne={activeOrigin.eTldPlusOne}
            />
          </SiteOriginTitle>
          <AccountsTitle>{getLocale('braveWalletSitePermissionsAccounts').replace('$1', connectedAccounts.length.toString())}</AccountsTitle>
        </HeaderColumn>
      </HeaderRow>
      <AddressContainer>
        <NewAccountButton onClick={onAddAccount}>{getLocale('braveWalletSitePermissionsNewAccount')}</NewAccountButton>
        <DividerLine />
        <AddressScrollContainer>
          {accountByCoinType.map((account) => (
            <ConnectedAccountItem
              key={account.accountId.uniqueKey}
              account={account}
            />
          ))}
        </AddressScrollContainer>
      </AddressContainer>
    </StyledWrapper>
  )
}

export default SitePermissions
