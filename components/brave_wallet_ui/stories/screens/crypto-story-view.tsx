// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { FilecoinAddressProtocol } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { StyledWrapper } from '../../components/desktop/views/crypto/style'
import {
  TopTabNavTypes,
  BraveWallet,
  AppsListType,
  PriceDataObjectType,
  AccountAssetOptionType,
  AccountTransactions,
  WalletAccountType,
  UpdateAccountNamePayloadType,
  DefaultCurrencies
} from '../../constants/types'
import { TopNavOptions } from '../../options/top-nav-options'
import { TopTabNav, WalletBanner, AddAccountModal } from '../../components/desktop'
import { SearchBar, AppList } from '../../components/shared'
import { getLocale } from '../../../common/locale'
import { AppsList } from '../../options/apps-list-options'
import { filterAppList } from '../../utils/filter-app-list'
import { PortfolioView, AccountsView } from '../../components/desktop/views'
import {
  HardwareWalletConnectOpts
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { HardwareWalletAccount } from './../../common/hardware/types'

export interface Props {
  defaultCurrencies: DefaultCurrencies
  isFetchingPortfolioPriceHistory: boolean
  selectedNetwork: BraveWallet.EthereumChain
  showAddModal: boolean
  isLoading: boolean
  userAssetList: AccountAssetOptionType[]
  transactions: AccountTransactions
  portfolioBalance: string
  selectedAsset: BraveWallet.ERCToken | undefined
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAssetPriceHistory: PriceDataObjectType[]
  portfolioPriceHistory: PriceDataObjectType[]
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  networkList: BraveWallet.EthereumChain[]
  accounts: WalletAccountType[]
  needsBackup: boolean
  userVisibleTokensInfo: BraveWallet.ERCToken[]
  fullAssetList: BraveWallet.ERCToken[]
  privateKey: string
  transactionSpotPrices: BraveWallet.AssetPrice[]
  hasImportError: boolean
  onAddUserAsset: (token: BraveWallet.ERCToken) => void
  onSetUserAssetVisible: (token: BraveWallet.ERCToken, isVisible: boolean) => void
  onRemoveUserAsset: (token: BraveWallet.ERCToken) => void
  onLockWallet: () => void
  onSetImportError: (hasError: boolean) => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onDoneViewingPrivateKey: () => void
  onViewPrivateKey: (address: string, isDefault: boolean) => void
  onRemoveAccount: (address: string, hardware: boolean) => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onToggleAddModal: () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  getBalance: (address: string) => Promise<string>
  onAddHardwareAccounts: (selected: HardwareWalletAccount[]) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<HardwareWalletAccount[]>
  onImportAccount: (accountName: string, privateKey: string) => void
  onImportFilecoinAccount: (accountName: string, privateKey: string, network: string, protocol: FilecoinAddressProtocol) => void
  onCreateAccount: (name: string) => void
  onSelectAsset: (asset: BraveWallet.ERCToken | undefined) => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onShowBackup: () => void
  onShowVisibleAssetsModal: (value: boolean) => void
  showVisibleAssetsModal: boolean
  onFindTokenInfoByContractAddress: (contractAddress: string) => void
  foundTokenInfoByContractAddress?: BraveWallet.ERCToken
}

const CryptoStoryView = (props: Props) => {
  const {
    defaultCurrencies,
    hasImportError,
    userVisibleTokensInfo,
    transactionSpotPrices,
    privateKey,
    selectedNetwork,
    fullAssetList,
    portfolioPriceHistory,
    userAssetList,
    selectedTimeline,
    selectedPortfolioTimeline,
    selectedAssetPriceHistory,
    needsBackup,
    accounts,
    networkList,
    selectedAsset,
    portfolioBalance,
    transactions,
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice,
    isLoading,
    showAddModal,
    isFetchingPortfolioPriceHistory,
    showVisibleAssetsModal,
    onShowVisibleAssetsModal,
    onAddUserAsset,
    onSetUserAssetVisible,
    onRemoveUserAsset,
    onLockWallet,
    onShowBackup,
    onChangeTimeline,
    onSelectAsset,
    onCreateAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccount,
    onImportFilecoinAccount,
    onUpdateAccountName,
    onSelectNetwork,
    onToggleAddModal,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onImportAccountFromJson,
    onSetImportError,
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = props
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)
  const [selectedAccount, setSelectedAccount] = React.useState<WalletAccountType>()
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList())
  const [favoriteApps, setFavoriteApps] = React.useState<BraveWallet.AppItem[]>([
    AppsList()[0].appList[0]
  ])
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')

  const browseMore = () => {
    alert('Will expand to view more!')
  }

  // In the future these will be actual paths
  // for example wallet/crypto/portfolio
  const tabTo = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  const addToFavorites = (app: BraveWallet.AppItem) => {
    const newList = [...favoriteApps, app]
    setFavoriteApps(newList)
  }
  const removeFromFavorites = (app: BraveWallet.AppItem) => {
    const newList = favoriteApps.filter(
      (fav) => fav.name !== app.name
    )
    setFavoriteApps(newList)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList(), setFilteredAppsList)
  }

  const toggleNav = () => {
    setHideNav(!hideNav)
  }

  const onDismissBackupWarning = () => {
    setShowBackupWarning(false)
  }

  const onClickAddAccount = () => {
    onToggleAddModal()
  }

  const onCloseAddModal = () => {
    onToggleAddModal()
  }

  const goBack = () => {
    setSelectedAccount(undefined)
    toggleNav()
  }

  const onSelectAccount = (account: WalletAccountType) => {
    setSelectedAccount(account)
    toggleNav()
  }

  const onDismissDefaultWalletBanner = () => {
    setShowDefaultWalletBanner(false)
  }

  const onClickSettings = () => {
    // Does nothing in storybook
    alert('Will Nav to brave://settings/wallet')
  }

  const onRouteBackToAccounts = () => {
    // Does nothing in storybook
  }

  const onClickRetryTransaction = () => {
    // Does nothing in storybook
    alert('Will retry transaction')
  }

  const onClickCancelTransaction = () => {
    // Does nothing in storybook
    alert('Will cancel transaction')
  }

  const onClickSpeedupTransaction = () => {
    // Does nothing in storybook
    alert('Will speedup transaction')
  }

  return (
    <StyledWrapper>
      {!hideNav &&
        <>
          <TopTabNav
            tabList={TopNavOptions()}
            selectedTab={selectedTab}
            onSubmit={tabTo}
            hasMoreButtons={true}
            onLockWallet={onLockWallet}
          />
          {showDefaultWalletBanner &&
            <WalletBanner
              description={getLocale('braveWalletDefaultWalletBanner')}
              onDismiss={onDismissDefaultWalletBanner}
              onClick={onClickSettings}
              bannerType='warning'
              buttonText={getLocale('braveWalletWalletPopupSettings')}
            />
          }

          {needsBackup && showBackupWarning &&
            <WalletBanner
              description={getLocale('braveWalletBackupWarningText')}
              onDismiss={onDismissBackupWarning}
              onClick={onShowBackup}
              bannerType='danger'
              buttonText={getLocale('braveWalletBackupButton')}
            />
          }
        </>
      }
      {selectedTab === 'apps' &&
        <>
          <SearchBar
            placeholder={getLocale('braveWalletSearchText')}
            action={filterList}
          />
          <AppList
            list={filteredAppsList}
            favApps={favoriteApps}
            addToFav={addToFavorites}
            removeFromFav={removeFromFavorites}
            action={browseMore}
          />
        </>
      }
      {selectedTab === 'portfolio' &&
        <PortfolioView
          defaultCurrencies={defaultCurrencies}
          toggleNav={toggleNav}
          accounts={accounts}
          networkList={networkList}
          onChangeTimeline={onChangeTimeline}
          selectedAssetPriceHistory={selectedAssetPriceHistory}
          selectedTimeline={selectedTimeline}
          selectedPortfolioTimeline={selectedPortfolioTimeline}
          onSelectAsset={onSelectAsset}
          onSelectAccount={onSelectAccount}
          onClickAddAccount={onClickAddAccount}
          onSelectNetwork={onSelectNetwork}
          addUserAssetError={false}
          onAddUserAsset={onAddUserAsset}
          onRemoveUserAsset={onRemoveUserAsset}
          onSetUserAssetVisible={onSetUserAssetVisible}
          selectedAsset={selectedAsset}
          portfolioBalance={portfolioBalance}
          portfolioPriceHistory={portfolioPriceHistory}
          transactions={transactions}
          selectedAssetFiatPrice={selectedAssetFiatPrice}
          selectedAssetCryptoPrice={selectedAssetCryptoPrice}
          userAssetList={userAssetList}
          isLoading={isLoading}
          selectedNetwork={selectedNetwork}
          fullAssetList={fullAssetList}
          userVisibleTokensInfo={userVisibleTokensInfo}
          isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
          transactionSpotPrices={transactionSpotPrices}
          onRetryTransaction={onClickRetryTransaction}
          onSpeedupTransaction={onClickSpeedupTransaction}
          onCancelTransaction={onClickCancelTransaction}
          onShowVisibleAssetsModal={onShowVisibleAssetsModal}
          showVisibleAssetsModal={showVisibleAssetsModal}
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
        />
      }
      {selectedTab === 'accounts' &&
        <AccountsView
          defaultCurrencies={defaultCurrencies}
          toggleNav={toggleNav}
          accounts={accounts}
          onClickBackup={onShowBackup}
          onClickAddAccount={onClickAddAccount}
          onUpdateAccountName={onUpdateAccountName}
          onRemoveAccount={onRemoveAccount}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          onViewPrivateKey={onViewPrivateKey}
          goBack={goBack}
          onSelectAccount={onSelectAccount}
          onSelectAsset={onSelectAsset}
          privateKey={privateKey}
          transactions={transactions}
          selectedNetwork={selectedNetwork}
          transactionSpotPrices={transactionSpotPrices}
          userVisibleTokensInfo={userVisibleTokensInfo}
          selectedAccount={selectedAccount}
          onRetryTransaction={onClickRetryTransaction}
          onSpeedupTransaction={onClickSpeedupTransaction}
          onCancelTransaction={onClickCancelTransaction}
        />
      }
      {showAddModal &&
        <AddAccountModal
          accounts={accounts}
          title={getLocale('braveWalletAddAccount')}
          onClose={onCloseAddModal}
          onCreateAccount={onCreateAccount}
          onImportAccount={onImportAccount}
          onImportFilecoinAccount={onImportFilecoinAccount}
          onConnectHardwareWallet={onConnectHardwareWallet}
          onAddHardwareAccounts={onAddHardwareAccounts}
          getBalance={getBalance}
          onImportAccountFromJson={onImportAccountFromJson}
          hasImportError={hasImportError}
          onSetImportError={onSetImportError}
          onRouteBackToAccounts={onRouteBackToAccounts}
        />
      }
    </StyledWrapper>
  )
}

export default CryptoStoryView
