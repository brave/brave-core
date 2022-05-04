// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { StyledWrapper } from '../../components/desktop/views/crypto/style'
import {
  TopTabNavTypes,
  BraveWallet,
  AppsListType,
  UserAssetInfoType,
  AccountTransactions,
  WalletAccountType,
  UpdateAccountNamePayloadType,
  DefaultCurrencies,
  AddAccountNavTypes
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

export interface Props {
  defaultCurrencies: DefaultCurrencies
  selectedNetwork: BraveWallet.NetworkInfo
  showAddModal: boolean
  userAssetList: UserAssetInfoType[]
  transactions: AccountTransactions
  networkList: BraveWallet.NetworkInfo[]
  accounts: WalletAccountType[]
  needsBackup: boolean
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  privateKey: string
  transactionSpotPrices: BraveWallet.AssetPrice[]
  hasImportError: boolean
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
  onLockWallet: () => void
  onSetImportError: (hasError: boolean) => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onDoneViewingPrivateKey: () => void
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
  onToggleAddModal: () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  getBalance: (address: string) => Promise<string>
  onAddHardwareAccounts: (selected: BraveWallet.HardwareWalletAccount[]) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<BraveWallet.HardwareWalletAccount[]>
  onImportAccount: (accountName: string, privateKey: string, coin: BraveWallet.CoinType) => void
  onImportFilecoinAccount: (accountName: string, privateKey: string, network: string) => void
  onCreateAccount: (name: string) => void
  onShowBackup: () => void
  onShowVisibleAssetsModal: (value: boolean) => void
  showVisibleAssetsModal: boolean
}

const CryptoStoryView = (props: Props) => {
  const {
    defaultCurrencies,
    hasImportError,
    userVisibleTokensInfo,
    transactionSpotPrices,
    privateKey,
    selectedNetwork,
    needsBackup,
    accounts,
    networkList,
    transactions,
    showAddModal,
    showVisibleAssetsModal,
    isFilecoinEnabled,
    isSolanaEnabled,
    onShowVisibleAssetsModal,
    onLockWallet,
    onShowBackup,
    onCreateAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccount,
    onImportFilecoinAccount,
    onUpdateAccountName,
    onToggleAddModal,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onImportAccountFromJson,
    onSetImportError
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
  const [addAccountModalTab, setAddAccountModalTab] = React.useState<AddAccountNavTypes>('create')
  const [showMore, setShowMore] = React.useState<boolean>(false)

  const browseMore = () => {
    alert('Will expand to view more!')
  }

  const onSelectTab = (path: TopTabNavTypes) => {
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

  const onClickAddAccount = (tabId: AddAccountNavTypes) => () => {
    setAddAccountModalTab(tabId)
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

  const onClickMore = () => {
    setShowMore(true)
  }

  const onHideMore = () => {
    if (showMore) {
      setShowMore(false)
    }
  }

  return (
    <StyledWrapper onClick={onHideMore}>
      {!hideNav &&
        <>
          <TopTabNav
            tabList={TopNavOptions()}
            selectedTab={selectedTab}
            onSelectTab={onSelectTab}
            hasMoreButtons={true}
            showMore={showMore}
            onClickLock={onLockWallet}
            onClickBackup={onShowBackup}
            onClickMore={onClickMore}
            onClickSettings={onClickSettings}
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
          toggleNav={toggleNav}
          onClickAddAccount={onClickAddAccount}
          onShowVisibleAssetsModal={onShowVisibleAssetsModal}
          showVisibleAssetsModal={showVisibleAssetsModal}
        />
      }
      {selectedTab === 'accounts' &&
        <AccountsView
          defaultCurrencies={defaultCurrencies}
          toggleNav={toggleNav}
          accounts={accounts}
          onClickAddAccount={onClickAddAccount}
          onUpdateAccountName={onUpdateAccountName}
          onRemoveAccount={onRemoveAccount}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          onViewPrivateKey={onViewPrivateKey}
          goBack={goBack}
          onSelectAccount={onSelectAccount}
          privateKey={privateKey}
          transactions={transactions}
          transactionSpotPrices={transactionSpotPrices}
          userVisibleTokensInfo={userVisibleTokensInfo}
          selectedAccount={selectedAccount}
          networkList={networkList}
        />
      }
      {showAddModal &&
        <AddAccountModal
          accounts={accounts}
          selectedNetwork={selectedNetwork}
          onClose={onCloseAddModal}
          onCreateAccount={onCreateAccount}
          onImportAccount={onImportAccount}
          onImportFilecoinAccount={onImportFilecoinAccount}
          isFilecoinEnabled={isFilecoinEnabled}
          isSolanaEnabled={isSolanaEnabled}
          onConnectHardwareWallet={onConnectHardwareWallet}
          onAddHardwareAccounts={onAddHardwareAccounts}
          getBalance={getBalance}
          onImportAccountFromJson={onImportAccountFromJson}
          hasImportError={hasImportError}
          onSetImportError={onSetImportError}
          tab={addAccountModalTab}
        />
      }
    </StyledWrapper>
  )
}

export default CryptoStoryView
