// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import * as React from 'react'
import { EntityId } from '@reduxjs/toolkit'
import Dropdown from '@brave/leo/react/dropdown'

// Types
import { BraveWallet } from '../../../constants/types'
import {
  DerivationScheme,
  HardwareImportScheme,
  AllHardwareImportSchemes,
  AccountFromDevice
} from '../../../common/hardware/types'

// Utils
import { getLocale } from '../../../../common/locale'

import {
  useGetNetworksRegistryQuery //
} from '../../../common/slices/api.slice'
import { makeNetworkAsset } from '../../../options/asset-options'
import {
  networkEntityAdapter //
} from '../../../common/slices/entities/network.entity'

// Components
import { SearchBar } from '../../shared/search-bar/index'
import { NetworkFilterSelector } from '../network-filter-selector'
import { AccountListItem } from './account_list_item'

// Styles
import {
  DisclaimerText,
  DisclaimerWrapper
} from '../popup-modals/add-account-modal/style'
import {
  ButtonsContainer,
  HardwareWalletAccountsListContainer,
  SelectRow,
  SelectWrapper,
  LoadingWrapper,
  LoadIcon,
  NoSearchResultText,
  AccountListContainer,
  AccountListHeader,
  AccountListContent,
  DropdownLabel,
  HelpLink
} from './hardware_wallet_connect.styles'
import {
  ContinueButton //
} from '../../../page/screens/onboarding/onboarding.style'
import { Row } from '../../shared/style'

interface Props {
  currentHardwareImportScheme: HardwareImportScheme
  accounts: Array<Required<AccountFromDevice>>
  onLoadMore: () => void
  onAccountChecked: (path: string, checked: boolean) => void
  setHardwareImportScheme: (scheme: DerivationScheme) => void
  onAddAccounts: () => void
}

const defaultNetworkId = (coin: BraveWallet.CoinType) => {
  if (coin === BraveWallet.CoinType.ETH) return BraveWallet.MAINNET_CHAIN_ID
  if (coin === BraveWallet.CoinType.SOL) return BraveWallet.SOLANA_MAINNET
  if (coin === BraveWallet.CoinType.BTC) return BraveWallet.BITCOIN_MAINNET
  if (coin === BraveWallet.CoinType.FIL) return BraveWallet.FILECOIN_MAINNET

  assertNotReached(`Unknown coin ${coin}`)
}

export const HardwareWalletAccountsList = ({
  currentHardwareImportScheme,
  setHardwareImportScheme,
  accounts,
  onLoadMore,
  onAccountChecked,
  onAddAccounts
}: Props) => {
  const coin = currentHardwareImportScheme.coin
  const hardwareVendor = currentHardwareImportScheme.vendor

  // queries
  const { data: networksRegistry } = useGetNetworksRegistryQuery()

  // state
  const [filteredAccountList, setFilteredAccountList] = React.useState<
    Array<Required<AccountFromDevice>>
  >([])
  const [isLoadingMore, setIsLoadingMore] = React.useState<boolean>(false)
  const [selectedNetworkId, setSelectedNetworkId] = React.useState<EntityId>(
    defaultNetworkId(coin)
  )

  // memos
  const accountNativeAsset = React.useMemo(() => {
    if (!networksRegistry) {
      return undefined
    }
    return makeNetworkAsset(networksRegistry.entities[selectedNetworkId])
  }, [networksRegistry, selectedNetworkId])

  const networksSubset = React.useMemo(() => {
    if (!networksRegistry) {
      return []
    }
    return networksRegistry.visibleIdsByCoinType[coin].map(
      (id) => networksRegistry.entities[id] as BraveWallet.NetworkInfo
    )
  }, [networksRegistry, coin])

  // methods
  const onSelectAccountCheckbox =
    (account: Required<AccountFromDevice>) => () => {
      onAccountChecked(account.derivationPath, !account.shouldAddToWallet)
    }

  const filterAccountList = (event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event?.target?.value || ''
    if (search === '') {
      setFilteredAccountList(accounts)
    } else {
      const filteredList = accounts.filter((account) => {
        return (
          account.address.toLowerCase() === search.toLowerCase() ||
          account.address.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredAccountList(filteredList)
    }
  }

  const onClickLoadMore = () => {
    setIsLoadingMore(true)
    onLoadMore()
  }

  const onSelectNetwork = React.useCallback(
    (n: BraveWallet.NetworkInfo): void => {
      setSelectedNetworkId(networkEntityAdapter.selectId(n))
      AllHardwareImportSchemes.map((scheme) => {
        if (scheme.coin === n.coin && scheme.fixedNetwork === n.chainId) {
          setHardwareImportScheme(scheme.derivationScheme)
        }
      })
    },
    [setSelectedNetworkId, setHardwareImportScheme]
  )

  const getHardwareImportSchemeLabel = (
    scheme: HardwareImportScheme
  ): string => {
    return `${scheme.name} "${scheme.pathTemplate('x')}"`
  }

  const supportedSchemes = AllHardwareImportSchemes.filter((scheme) => {
    return scheme.coin === coin && scheme.vendor === hardwareVendor
  })

  const onChangeDerivationScheme = (value?: string) => {
    if (value) {
      setHardwareImportScheme(value as DerivationScheme)
    }
  }

  // effects
  React.useEffect(() => {
    setFilteredAccountList(accounts)
    setIsLoadingMore(false)
  }, [accounts])

  React.useEffect(() => {
    if (selectedNetworkId) {
      return
    }
    if (!networksRegistry) {
      return
    }

    // set network dropdown default value
    setSelectedNetworkId(networksRegistry.visibleIdsByCoinType[coin][0])
  }, [networksRegistry, coin, selectedNetworkId])

  // render
  return (
    <>
      <SelectRow>
        <SelectWrapper>
          <NetworkFilterSelector
            networkListSubset={networksSubset}
            selectedNetwork={networksRegistry?.entities[selectedNetworkId]}
            onSelectNetwork={onSelectNetwork}
            disableAllAccountsOption
            isV2
          />
          {coin === BraveWallet.CoinType.ETH ? (
            <Dropdown
              value={currentHardwareImportScheme.derivationScheme}
              onChange={(e) => onChangeDerivationScheme(e.value)}
            >
              <Row
                width='100%'
                justifyContent='space-between'
                slot='label'
              >
                <DropdownLabel>{getLocale('braveWalletHDPath')}</DropdownLabel>
                <HelpLink
                  href='https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  {getLocale('braveWalletHelpCenter')}
                </HelpLink>
              </Row>
              <div slot='value'>
                {getHardwareImportSchemeLabel(currentHardwareImportScheme)}
              </div>
              {supportedSchemes.map((scheme) => {
                return (
                  <leo-option
                    value={scheme.derivationScheme}
                    key={scheme.derivationScheme}
                  >
                    {getHardwareImportSchemeLabel(scheme)}
                  </leo-option>
                )
              })}
            </Dropdown>
          ) : null}
          {coin === BraveWallet.CoinType.SOL ? (
            <Dropdown
              value={currentHardwareImportScheme.derivationScheme}
              onChange={(e) => onChangeDerivationScheme(e.value)}
            >
              <div slot='value'>
                {getHardwareImportSchemeLabel(currentHardwareImportScheme)}
              </div>
              {supportedSchemes.map((scheme) => {
                return (
                  <leo-option
                    value={scheme.derivationScheme}
                    key={scheme.derivationScheme}
                  >
                    {getHardwareImportSchemeLabel(scheme)}
                  </leo-option>
                )
              })}
            </Dropdown>
          ) : null}
        </SelectWrapper>
      </SelectRow>
      {coin !== BraveWallet.CoinType.FIL && (
        <DisclaimerWrapper>
          <DisclaimerText>
            {getLocale('braveWalletSwitchHDPathTextHardwareWallet')}
          </DisclaimerText>
        </DisclaimerWrapper>
      )}
      <SearchBar
        placeholder={getLocale('braveWalletSearchScannedAccounts')}
        action={filterAccountList}
        isV2
      />
      <HardwareWalletAccountsListContainer>
        {accounts.length === 0 && (
          <LoadingWrapper>
            <LoadIcon size={'big'} />
          </LoadingWrapper>
        )}

        {accounts.length > 0 && filteredAccountList.length === 0 && (
          <NoSearchResultText>
            {getLocale('braveWalletConnectHardwareSearchNothingFound')}
          </NoSearchResultText>
        )}

        {accountNativeAsset &&
          accounts.length > 0 &&
          filteredAccountList.length > 0 && (
            <AccountListContainer>
              <AccountListHeader>
                <div>{getLocale('braveWalletSubviewAccount')}</div>
                <div>{getLocale('braveWalletBalance')}</div>
                <div>{getLocale('braveWalletAddAccountConnect')}</div>
              </AccountListHeader>
              <AccountListContent>
                {filteredAccountList.map((account) => {
                  return (
                    <AccountListItem
                      key={account.derivationPath}
                      balanceAsset={accountNativeAsset}
                      address={account.address}
                      selected={
                        account.alreadyInWallet || account.shouldAddToWallet
                      }
                      disabled={account.alreadyInWallet}
                      onSelect={onSelectAccountCheckbox(account)}
                    />
                  )
                })}
              </AccountListContent>
            </AccountListContainer>
          )}
      </HardwareWalletAccountsListContainer>
      <ButtonsContainer>
        <ContinueButton
          onClick={onClickLoadMore}
          isLoading={isLoadingMore}
          isDisabled={
            isLoadingMore ||
            accounts.length === 0 ||
            currentHardwareImportScheme.singleAccount
          }
        >
          {isLoadingMore
            ? getLocale('braveWalletLoadingMoreAccountsHardwareWallet')
            : getLocale('braveWalletLoadMoreAccountsHardwareWallet')}
        </ContinueButton>
        <ContinueButton
          onClick={onAddAccounts}
          isDisabled={!accounts.find((acc) => acc.shouldAddToWallet)}
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </ButtonsContainer>
    </>
  )
}

export default HardwareWalletAccountsList
