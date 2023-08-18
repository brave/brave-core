// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  BraveWallet,
  SupportedTestNetworks
} from '../../../../constants/types'

// Selectors
import { WalletSelectors } from '../../../../common/selectors'
import {
  useUnsafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'

// Styled Components
import {
  ConnectPanelButton,
  AccountAddressText,
  AccountNameText,
  BalanceText,
  NameAndAddressColumn,
  AccountCircle,
  LeftSide,
  SelectedIcon
} from './select-account-item.style'
import { LoadingSkeleton } from '../../../shared/loading-skeleton/index'
import { Tooltip } from '../../../shared/tooltip/index'

// Utils
import { reduceAccountDisplayName } from '../../../../utils/reduce-account-name'
import { reduceAddress } from '../../../../utils/reduce-address'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { getBalance } from '../../../../utils/balance-utils'
import Amount from '../../../../utils/amount'
import { getPriceIdForToken } from '../../../../utils/api-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworksQuery,
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../../common/slices/constants'
import {
  TokenBalancesRegistry
} from '../../../../common/slices/entities/token-balance.entity'

// Hooks
import { useAccountOrb } from '../../../../common/hooks/use-orb'

interface Props {
  account: BraveWallet.AccountInfo
  isSelected: boolean
  onSelectAccount: () => void
  tokenBalancesRegistry: TokenBalancesRegistry | undefined
}

export const SelectAccountItem = (props: Props) => {
  const { account, isSelected, onSelectAccount, tokenBalancesRegistry } = props

  // Wallet Selectors
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: networks = [] } = useGetNetworksQuery()

  // Memos
  const orb = useAccountOrb(account)

  const tokenListByAccount = React.useMemo(() => {
    if (
      selectedNetwork?.coin &&
      selectedNetwork?.chainId &&
      SupportedTestNetworks.includes(selectedNetwork.chainId)
    ) {
      return userVisibleTokensInfo.filter(
        (token) =>
          token.visible &&
          !token.isErc721 &&
          !token.isErc1155 &&
          !token.isNft &&
          token.chainId === selectedNetwork.chainId &&
          token.coin === selectedNetwork.coin
      )
    }
    const chainList =
      networks
        .filter(
          (network) =>
            network.coin === account.accountId.coin &&
            !SupportedTestNetworks.includes(network.chainId)
        )
        .map((network) => network.chainId) ?? []
    return userVisibleTokensInfo.filter(
      (token) =>
        token.visible &&
        !token.isErc721 &&
        !token.isErc1155 &&
        !token.isNft &&
        chainList.includes(token.chainId)
    )
  }, [userVisibleTokensInfo, networks, account, selectedNetwork?.coin, selectedNetwork?.chainId])



  const tokenPriceIds = React.useMemo(() =>
    tokenListByAccount.map(getPriceIdForToken),
    [tokenListByAccount]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  const accountFiatValue = React.useMemo(() => {
    const amounts = tokenListByAccount.map((token) => {
      const balance = getBalance(
        account.accountId,
        token,
        tokenBalancesRegistry
      )

      return computeFiatAmount({
        spotPriceRegistry,
        value: balance,
        token
      }).format()
    })

    if (amounts.length === 0) {
      return ''
    }

    const reducedAmounts = amounts.reduce(function (a, b) {
      return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
    })
    return new Amount(reducedAmounts).formatAsFiat(defaultFiatCurrency)
  }, [
    tokenListByAccount,
    spotPriceRegistry,
    defaultFiatCurrency,
    tokenBalancesRegistry
  ])

  return (
    <ConnectPanelButton border='top' onClick={onSelectAccount}>
      <LeftSide>
        <AccountCircle orb={orb} />
        <NameAndAddressColumn>
          <AccountNameText>
            {reduceAccountDisplayName(account.name, 22)}
          </AccountNameText>
          {account.address && (
            <Tooltip
              isAddress={true}
              minWidth={120}
              maxWidth={120}
              text={account.address}
            >
              <AccountAddressText>
                {reduceAddress(account.address)}
              </AccountAddressText>
            </Tooltip>
          )}
          {accountFiatValue === '' ? (
            <LoadingSkeleton width={60} height={18} />
          ) : (
            <BalanceText>{accountFiatValue}</BalanceText>
          )}
        </NameAndAddressColumn>
      </LeftSide>
      <SelectedIcon
        name={isSelected ? 'radio-checked' : 'radio-unchecked'}
        isSelected={isSelected}
      />
    </ConnectPanelButton>
  )
}

export default SelectAccountItem
