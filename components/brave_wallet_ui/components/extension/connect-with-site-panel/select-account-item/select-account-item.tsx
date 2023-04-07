// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Types
import {
  WalletAccountType,
  SupportedTestNetworks
} from '../../../../constants/types'

// Selectors
import { WalletSelectors } from '../../../../common/selectors'
import { useUnsafeWalletSelector, useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'

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
import { LoadingSkeleton, Tooltip } from '../../../shared'

// Utils
import { reduceAccountDisplayName } from '../../../../utils/reduce-account-name'
import { reduceAddress } from '../../../../utils/reduce-address'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { getBalance } from '../../../../utils/balance-utils'
import Amount from '../../../../utils/amount'
import {
  useGetNetworksQuery,
  useGetSelectedChainQuery
} from '../../../../common/slices/api.slice'

interface Props {
  account: WalletAccountType
  isSelected: boolean
  onSelectAccount: () => void
}
export const SelectAccountItem = (props: Props) => {
  const { account, isSelected, onSelectAccount } = props

  // Wallet Selectors
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const spotPrices = useUnsafeWalletSelector(
    WalletSelectors.transactionSpotPrices
  )
  const defaultFiatCurrency = useSafeWalletSelector(
    WalletSelectors.defaultFiatCurrency
  )

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: networks = [] } = useGetNetworksQuery()

  // Memos
  const orb = React.useMemo(() => {
    return create({
      seed: account.address.toLowerCase(),
      size: 8,
      scale: 16
    }).toDataURL()
  }, [account.address])

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
          !token.isNft &&
          token.chainId === selectedNetwork.chainId &&
          token.coin === selectedNetwork.coin
      )
    }
    const chainList =
      networks
        .filter(
          (network) =>
            network.coin === account.coin &&
            !SupportedTestNetworks.includes(network.chainId)
        )
        .map((network) => network.chainId) ?? []
    return userVisibleTokensInfo.filter(
      (token) =>
        token.visible &&
        !token.isErc721 &&
        !token.isNft &&
        chainList.includes(token.chainId)
    )
  }, [userVisibleTokensInfo, networks, account.coin, selectedNetwork?.coin, selectedNetwork?.chainId])

  const accountFiatValue = React.useMemo(() => {
    const amounts = tokenListByAccount.map((token) => {
      const balance = getBalance(account, token)
      return computeFiatAmount(spotPrices, {
        decimals: token.decimals,
        symbol: token.symbol,
        value: balance,
        contractAddress: token.contractAddress,
        chainId: token.chainId
      }).format()
    })

    if (amounts.length === 0) {
      return ''
    }

    const reducedAmounts = amounts.reduce(function (a, b) {
      return a !== '' && b !== '' ? new Amount(a).plus(b).format() : ''
    })
    return new Amount(reducedAmounts).formatAsFiat(defaultFiatCurrency)
  }, [tokenListByAccount, spotPrices, defaultFiatCurrency])

  return (
    <ConnectPanelButton border="top" onClick={onSelectAccount}>
      <LeftSide>
        <AccountCircle orb={orb} />
        <NameAndAddressColumn>
          <AccountNameText>
            {reduceAccountDisplayName(account.name, 22)}
          </AccountNameText>
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
