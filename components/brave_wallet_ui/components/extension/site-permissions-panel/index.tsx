// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useDispatch,
  useSelector
} from 'react-redux'

// Actions
import { PanelActions } from '../../../panel/actions'

// Types
import { BraveWallet, WalletAccountType, WalletState } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'

// Hooks
import { useIsMounted } from '../../../common/hooks'

// Components
import {
  DividerLine,
  ConnectedAccountItem
} from '../'
import { CreateSiteOrigin } from '../../shared'

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
  const {
    accounts,
    connectedAccounts,
    activeOrigin,
    selectedCoin
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // hooks
  const isMounted = useIsMounted()

  // state
  const [connectedAccountsTitle, setConnectedAccountsTitle] = React.useState<string>('')

  // methods
  const onAddAccount = React.useCallback(() => {
    dispatch(PanelActions.expandWalletAccounts())
  }, [])

  // memos
  const accountByCoinType = React.useMemo((): WalletAccountType[] => {
    return accounts.filter((account) => account.coin === selectedCoin)
  }, [accounts, selectedCoin])

  const connectedAccountsTitleStringId = React.useMemo((): string => {
    return selectedCoin === BraveWallet.CoinType.SOL
      ? 'braveWalletSitePermissionsAccountsTrusted'
      : 'braveWalletSitePermissionsAccountsConnected'
  }, [selectedCoin])

  // effects
  React.useEffect(() => {
    const getString = async () => {
      await PluralStringProxyImpl
        .getInstance()
        .getPluralString(connectedAccountsTitleStringId, connectedAccounts.length)
        .then(result => {
          if (isMounted) {
            setConnectedAccountsTitle(result)
          }
        })
        .catch(e => console.log(e))
    }
    getString()
  }, [selectedCoin, connectedAccounts, connectedAccountsTitleStringId, isMounted])

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
          <AccountsTitle>{connectedAccountsTitle}</AccountsTitle>
        </HeaderColumn>
      </HeaderRow>
      <AddressContainer>
        <NewAccountButton onClick={onAddAccount}>{getLocale('braveWalletSitePermissionsNewAccount')}</NewAccountButton>
        <DividerLine />
        <AddressScrollContainer>
          {accountByCoinType.map((account) => (
            <ConnectedAccountItem
              key={account.id}
              account={account}
            />
          ))}
        </AddressScrollContainer>
      </AddressContainer>
    </StyledWrapper>
  )
}

export default SitePermissions
