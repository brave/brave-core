// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Selectors
import { WalletSelectors } from '../../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'

// Actions
import { WalletActions } from '../../../../../common/actions/'

// Types
import { BraveWallet, WalletAccountType, CoinTypesMap, SendOptionTypes } from '../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../common/locale'
import {
  getFilecoinKeyringIdFromNetwork //
} from '../../../../../utils/network-utils'
import { getBalance } from '../../../../../utils/balance-utils'
import { computeFiatAmount } from '../../../../../utils/pricing-utils'
import Amount from '../../../../../utils/amount'
import {
  useGetVisibleNetworksQuery,
  useSetNetworkMutation
} from '../../../../../common/slices/api.slice'

// Options
import { AllNetworksOption } from '../../../../../options/network-filter-options'

// Assets
import CloseIcon from '../../../../../assets/svg-icons/close.svg'

// Components
import { TokenListItem } from '../token-list-item/token-list-item'
import { NetworkFilterWithSearch } from '../../../../../components/desktop/network-filter-with-search'

// Styled Components
import { Row, Column, Text, VerticalDivider, IconButton, VerticalSpacer } from '../../shared.styles'
import { Wrapper, Modal, ScrollContainer, AccountSection } from './select-tokenmodal.style'

interface Props {
  onClose: () => void
  selectedSendOption: SendOptionTypes
  selectSendAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
}

export const SelectTokenModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const { onClose, selectedSendOption, selectSendAsset } = props

    // Redux
    const dispatch = useDispatch()

    // Wallet Selectors
    const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
    const userVisibleTokensInfo = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
    const spotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
    const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

    // State
    const [searchValue, setSearchValue] = React.useState<string>('')
    const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)
    const [selectedNetworkFilter, setSelectedNetworkFilter] = React.useState<BraveWallet.NetworkInfo>(AllNetworksOption)

    // Queries & Mutations
    const [setNetwork] = useSetNetworkMutation()
    const { data: networks } = useGetVisibleNetworksQuery()

    // Methods
    const getTokenListByAccount = React.useCallback((account: WalletAccountType) => {
      if (!account || !networks) {
        return []
      }
      // Since LOCALHOST's chainId is shared between coinType's
      // this check will make sure we are returning the correct
      // LOCALHOST asset for each account.
      const coinName = CoinTypesMap[account?.coin ?? 0]
      const localHostCoins = userVisibleTokensInfo.filter((token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
      const accountsLocalHost = localHostCoins.find((token) => token.symbol.toUpperCase() === coinName)

      const chainList = networks
        .filter(
          (network) =>
            network.coin === account?.coin &&
            (network.coin !== BraveWallet.CoinType.FIL ||
              getFilecoinKeyringIdFromNetwork(network) === account?.keyringId)
        )
        .map((network) => network.chainId)

      const list = userVisibleTokensInfo.filter(
        (token) =>
          token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID &&
          chainList.includes(token?.chainId ?? '')
      )

      if (accountsLocalHost && (account.keyringId !== BraveWallet.FILECOIN_KEYRING_ID)) {
        list.push(accountsLocalHost)
        return list
      }

      return list.filter((token) => token.visible)
    }, [userVisibleTokensInfo, networks])

    const getTokenListWithBalances = React.useCallback((account: WalletAccountType) => {
      return getTokenListByAccount(account).filter((token) => getBalance(account, token) > '0')
    }, [getTokenListByAccount])

    const getTokensBySelectedSendOption = React.useCallback((account: WalletAccountType) => {
      if (selectedSendOption === 'nft') {
        return getTokenListWithBalances(account).filter(token => token.isErc721 || token.isNft)
      }
      return getTokenListWithBalances(account).filter(token => !token.isErc721 && !token.isNft)
    }, [getTokenListWithBalances, selectedSendOption])

    const getTokensByNetwork = React.useCallback((account: WalletAccountType) => {
      if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
        return getTokensBySelectedSendOption(account)
      }
      return getTokensBySelectedSendOption(account).filter((token) =>
        token.chainId === selectedNetworkFilter.chainId &&
        token.coin === selectedNetworkFilter.coin
      )
    }, [getTokensBySelectedSendOption, selectedNetworkFilter.chainId, selectedNetworkFilter.coin])

    const getTokensBySearchValue = React.useCallback((account: WalletAccountType) => {
      if (searchValue === '') {
        return getTokensByNetwork(account)
      }
      return getTokensByNetwork(account).filter((token) =>
        token.name.toLowerCase() === searchValue.toLowerCase() ||
        token.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        token.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        token.symbol.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        token.contractAddress.toLocaleLowerCase() === searchValue.toLowerCase()
      )
    }, [getTokensByNetwork, searchValue])

    const getAccountFiatValue = React.useCallback((account: WalletAccountType) => {
      const amounts = getTokensBySearchValue(account).map((token) => {
        const balance = getBalance(account, token)
        return computeFiatAmount(spotPrices, {
          decimals: token.decimals,
          symbol: token.symbol,
          value: balance,
          contractAddress: token.contractAddress,
          chainId: token.chainId
        }).format()
      })
      const reducedAmounts = amounts.reduce(function (a, b) {
        return a !== '' && b !== ''
          ? new Amount(a).plus(b).format()
          : ''
      })
      return new Amount(reducedAmounts).formatAsFiat(defaultCurrencies.fiat)
    }, [getTokensBySearchValue, spotPrices, defaultCurrencies.fiat])

    const onSelectSendAsset = React.useCallback(
      async (
        token: BraveWallet.BlockchainToken,
        account: WalletAccountType
      ) => {
        selectSendAsset(token)
        dispatch(WalletActions.selectAccount(account))
        await setNetwork({
          chainId: token.chainId,
          coin: token.coin
        })
        onClose()
      },
      [selectSendAsset, onClose, setNetwork]
    )

    const onSearch = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    }, [])

    const onToggleShowNetworkDropdown = React.useCallback(() => {
      setShowNetworkDropDown((prev) => !prev)
    }, [])

    const onSelectAssetsNetwork = React.useCallback((network: BraveWallet.NetworkInfo) => {
      setSelectedNetworkFilter(network)
      setShowNetworkDropDown(false)
    }, [])

    // Memos
    const emptyTokensList = React.useMemo(() => {
      return accounts.map((account) => getTokensBySearchValue(account)).flat(1).length === 0
    }, [accounts, getTokensBySearchValue])

    const modalTitle = React.useMemo(() => {
      if (selectedSendOption === 'nft') {
        return getLocale('braveWalletSendTabSelectNFTTitle')
      }
      return getLocale('braveWalletSendTabSelectTokenTitle')
    }, [selectedSendOption])

    const tokensByAccount = React.useMemo(() => {
      if (emptyTokensList) {
        return <Text textSize='14px' isBold={false} textColor='text03'>
          {getLocale('braveWalletNoAvailableTokens')}
        </Text>
      }
      return accounts.map((account) =>
        getTokensBySearchValue(account).length > 0 &&
        <Column columnWidth='full' key={account.name}>
          <AccountSection
            rowWidth='full'
            verticalPadding={6}
            horizontalPadding={16}
          >
            <Text
              textColor='text02'
              textSize='14px'
              isBold={true}
            >
              {account.name}
            </Text>
            {selectedSendOption === 'token' &&
              <Text
                textColor='text03'
                textSize='14px'
                isBold={false}
              >
                {getAccountFiatValue(account)}
              </Text>
            }
          </AccountSection>
          <Column columnWidth='full' horizontalPadding={8}>
            <VerticalSpacer size={8} />
            {getTokensBySearchValue(account).map((token) =>
              <TokenListItem
                token={token}
                onClick={() => onSelectSendAsset(token, account)}
                key={`${token.contractAddress}-${token.chainId}-${token.tokenId}`}
                balance={getBalance(account, token)}
              />
            )}
          </Column>
        </Column>
      )
    }, [
      accounts,
      onSelectSendAsset,
      getTokensBySearchValue,
      getAccountFiatValue,
      emptyTokensList,
      selectedSendOption
    ])

    // render
    return (
      <Wrapper>
        <Modal ref={forwardedRef}>
          <Row rowWidth='full' horizontalPadding={24} verticalPadding={20}>
            <Text textSize='18px' isBold={true}>
              {modalTitle}
            </Text>
            <IconButton icon={CloseIcon} onClick={onClose} size={20} />
          </Row>
          <Row rowWidth='full' horizontalPadding={16} marginBottom={16}>
            <NetworkFilterWithSearch
              searchValue={searchValue}
              searchPlaceholder={
                selectedSendOption === 'token'
                  ? getLocale('braveWalletSearchTokens')
                  : getLocale('braveWalletSearchNFTs')
              }
              searchAction={onSearch}
              searchAutoFocus={true}
              selectedNetwork={selectedNetworkFilter}
              onClick={onToggleShowNetworkDropdown}
              showNetworkDropDown={showNetworkDropDown}
              onSelectNetwork={onSelectAssetsNetwork}
            />
          </Row>
          <VerticalDivider />
          <ScrollContainer
            columnWidth='full'
            verticalAlign={emptyTokensList ? 'center' : 'flex-start'}
          >
            {tokensByAccount}
          </ScrollContainer>
        </Modal>
      </Wrapper>
    )
  }
)
