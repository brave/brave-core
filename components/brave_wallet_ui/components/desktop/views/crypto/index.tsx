// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Route, useHistory, useParams, Switch } from 'react-router-dom'

// actions
import { WalletPageActions } from '../../../../page/actions'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import {
  BraveWallet,
  TopTabNavTypes,
  WalletAccountType,
  UpdateAccountNamePayloadType,
  WalletRoutes,
  AddAccountNavTypes
} from '../../../../constants/types'
import { TOP_NAV_OPTIONS } from '../../../../options/top-nav-options'
import { HardwareWalletConnectOpts } from '../../popup-modals/add-account-modal/hardware-wallet-connect/types'

// style
import { StyledWrapper } from './style'

// components
import { TopTabNav, WalletBanner, AddAccountModal, EditVisibleAssetsModal } from '../../'
import { PortfolioOverview } from '../portfolio/portfolio-overview'
import { PortfolioAsset } from '../portfolio/portfolio-asset'
import { Accounts } from '../accounts/accounts'
import { Account } from '../accounts/account'

interface ParamsType {
  category?: TopTabNavTypes
  id?: string
}

export interface Props {
  onCreateAccount: (name: string, coin: BraveWallet.CoinType) => void
  onImportAccount: (accountName: string, privateKey: string, coin: BraveWallet.CoinType) => void
  onImportFilecoinAccount: (accountName: string, key: string, network: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<BraveWallet.HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: BraveWallet.HardwareWalletAccount[]) => void
  getBalance: (address: string, coin: BraveWallet.CoinType) => Promise<string>
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onDoneViewingPrivateKey: () => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onSetImportError: (error: boolean) => void
  onOpenWalletSettings: () => void
  hasImportError: boolean
  needsBackup: boolean
  accounts: WalletAccountType[]
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
  selectedNetwork: BraveWallet.NetworkInfo
  defaultWallet: BraveWallet.DefaultWallet
  isMetaMaskInstalled: boolean
}

const CryptoView = (props: Props) => {
  const {
    onCreateAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccount,
    onImportFilecoinAccount,
    onUpdateAccountName,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onImportAccountFromJson,
    onSetImportError,
    onOpenWalletSettings,
    defaultWallet,
    hasImportError,
    selectedNetwork,
    needsBackup,
    accounts,
    isFilecoinEnabled,
    isSolanaEnabled,
    isMetaMaskInstalled
  } = props

  // state
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [addAccountModalTab, setAddAccountModalTab] = React.useState<AddAccountNavTypes>('create')

  // routing
  const history = useHistory()
  const { category } = useParams<ParamsType>()

  // redux
  const dispatch = useDispatch()

  // methods
  const onShowBackup = React.useCallback(() => {
    history.push(WalletRoutes.Backup)
  }, [])

  const onShowVisibleAssetsModal = React.useCallback((showModal: boolean) => {
    if (showModal) {
      history.push(`${WalletRoutes.AddAssetModal}`)
    } else {
      history.push(`${WalletRoutes.Portfolio}`)
    }
  }, [])

  const onSelectTab = React.useCallback((path: TopTabNavTypes) => {
    history.push(`/crypto/${path}`)
  }, [])

  const toggleNav = React.useCallback(() => {
    setHideNav(!hideNav)
  }, [hideNav])

  const onDismissBackupWarning = React.useCallback(() => {
    setShowBackupWarning(false)
  }, [])

  const onDismissDefaultWalletBanner = React.useCallback(() => {
    setShowDefaultWalletBanner(false)
  }, [])

  const onCloseAddModal = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
    dispatch(WalletPageActions.setShowAddModal(false))
  }, [])

  const onClickAddAccount = React.useCallback((tabId: AddAccountNavTypes) => () => {
    setAddAccountModalTab(tabId)
    dispatch(WalletPageActions.setShowAddModal(true))
    history.push(WalletRoutes.AddAccountModal)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
    setHideNav(false)
  }, [])

  const onSelectAccount = React.useCallback((account: WalletAccountType | undefined) => {
    if (account) {
      history.push(`${WalletRoutes.Accounts}/${account.address}`)
    }
  }, [])

  const onClickSettings = React.useCallback(() => {
    chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

  const onClickShowMore = React.useCallback(() => {
    setShowMore(true)
  }, [])

  const onClickHideMore = React.useCallback(() => {
    if (showMore) {
      setShowMore(false)
    }
  }, [showMore])

  const hideVisibleAssetsModal = React.useCallback(() => onShowVisibleAssetsModal(false), [])

  // memos
  const nav = React.useMemo(() => (
    <>
      <TopTabNav
        selectedTab={category}
        showMore={showMore}
        hasMoreButtons={true}
        onSelectTab={onSelectTab}
        tabList={TOP_NAV_OPTIONS}
        onClickBackup={onShowBackup}
        onClickSettings={onClickSettings}
        onClickMore={onClickShowMore}
      />
      {(defaultWallet !== BraveWallet.DefaultWallet.BraveWallet &&
        (defaultWallet !== BraveWallet.DefaultWallet.BraveWalletPreferExtension || (defaultWallet === BraveWallet.DefaultWallet.BraveWalletPreferExtension && isMetaMaskInstalled))) &&
        showDefaultWalletBanner &&
        <WalletBanner
          onDismiss={onDismissDefaultWalletBanner}
          onClick={onOpenWalletSettings}
          bannerType='warning'
          buttonText={getLocale('braveWalletWalletPopupSettings')}
          description={getLocale('braveWalletDefaultWalletBanner')}
        />
      }
      {needsBackup && showBackupWarning &&
        <WalletBanner
          onDismiss={onDismissBackupWarning}
          onClick={onShowBackup}
          bannerType='danger'
          buttonText={getLocale('braveWalletBackupButton')}
          description={getLocale('braveWalletBackupWarningText')}
        />
      }
    </>
  ), [
    category,
    defaultWallet,
    isMetaMaskInstalled,
    needsBackup,
    onClickSettings,
    onClickShowMore,
    onDismissBackupWarning,
    onDismissDefaultWalletBanner,
    onOpenWalletSettings,
    onSelectTab,
    onShowBackup,
    showBackupWarning,
    showDefaultWalletBanner,
    showMore
  ])

  // render
  return (
    <StyledWrapper onClick={onClickHideMore}>
      <Switch>
        {/* Portfolio */}
        <Route path={WalletRoutes.AddAssetModal} exact>
          {nav}
          <PortfolioOverview />
          <EditVisibleAssetsModal
            onClose={hideVisibleAssetsModal}
          />
        </Route>

        <Route path={WalletRoutes.Portfolio} exact>
          {nav}
          <PortfolioOverview />
        </Route>

        <Route path={WalletRoutes.PortfolioSub} exact>
          <PortfolioAsset onClickAddAccount={onClickAddAccount} />
        </Route>

        {/* Accounts */}
        <Route path={WalletRoutes.AddAccountModal} exact>
          {nav}
          <Accounts
            onClickAddAccount={onClickAddAccount}
            onRemoveAccount={onRemoveAccount}
            onSelectAccount={onSelectAccount}
          />
          <AddAccountModal
            accounts={accounts}
            selectedNetwork={selectedNetwork}
            onClose={onCloseAddModal}
            onCreateAccount={onCreateAccount}
            onImportAccount={onImportAccount}
            isFilecoinEnabled={isFilecoinEnabled}
            isSolanaEnabled={isSolanaEnabled}
            onImportFilecoinAccount={onImportFilecoinAccount}
            onConnectHardwareWallet={onConnectHardwareWallet}
            onAddHardwareAccounts={onAddHardwareAccounts}
            getBalance={getBalance}
            onImportAccountFromJson={onImportAccountFromJson}
            hasImportError={hasImportError}
            onSetImportError={onSetImportError}
            tab={addAccountModalTab}
          />
        </Route>

        <Route path={WalletRoutes.Accounts} exact>
          {nav}
          <Accounts
            onClickAddAccount={onClickAddAccount}
            onRemoveAccount={onRemoveAccount}
            onSelectAccount={onSelectAccount}
          />
        </Route>

        <Route path={WalletRoutes.AccountsSub} exact>
          <Account
            toggleNav={toggleNav}
            onUpdateAccountName={onUpdateAccountName}
            onRemoveAccount={onRemoveAccount}
            onDoneViewingPrivateKey={onDoneViewingPrivateKey}
            onViewPrivateKey={onViewPrivateKey}
            goBack={goBack}
          />
        </Route>

      </Switch>
    </StyledWrapper>
  )
}

export default CryptoView
