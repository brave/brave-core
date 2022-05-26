// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, useHistory, useParams, Switch, Redirect } from 'react-router-dom'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import {
  BraveWallet,
  TopTabNavTypes,
  UpdateAccountNamePayloadType,
  WalletRoutes
} from '../../../../constants/types'
import { TOP_NAV_OPTIONS } from '../../../../options/top-nav-options'

// style
import { StyledWrapper } from './style'

// components
import { TopTabNav, WalletBanner, EditVisibleAssetsModal } from '../../'
import { PortfolioOverview } from '../portfolio/portfolio-overview'
import { PortfolioAsset } from '../portfolio/portfolio-asset'
import { Accounts } from '../accounts/accounts'
import { Account } from '../accounts/account'
import { AddAccountModal } from '../../popup-modals/add-account-modal/add-account-modal'

interface ParamsType {
  category?: TopTabNavTypes
  id?: string
}

export interface Props {
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onDoneViewingPrivateKey: () => void
  onOpenWalletSettings: () => void
  needsBackup: boolean
  defaultEthereumWallet: BraveWallet.DefaultWallet
  defaultSolanaWallet: BraveWallet.DefaultWallet
  isMetaMaskInstalled: boolean
  sessionRoute: string | undefined
}

const CryptoView = (props: Props) => {
  const {
    onUpdateAccountName,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onOpenWalletSettings,
    defaultEthereumWallet,
    defaultSolanaWallet,
    needsBackup,
    isMetaMaskInstalled,
    sessionRoute
  } = props

  // state
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)
  const [showMore, setShowMore] = React.useState<boolean>(false)

  // routing
  const history = useHistory()
  const { category } = useParams<ParamsType>()

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

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
    setHideNav(false)
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

  const hideVisibleAssetsModal = React.useCallback(
    () => onShowVisibleAssetsModal(false),
    [onShowVisibleAssetsModal]
  )

  const showBanner = React.useMemo((): boolean => {
    return (
      (defaultEthereumWallet !== BraveWallet.DefaultWallet.BraveWallet ||
        defaultSolanaWallet !== BraveWallet.DefaultWallet.BraveWallet) &&
      (defaultEthereumWallet !== BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
        defaultSolanaWallet !== BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
        (defaultEthereumWallet === BraveWallet.DefaultWallet.BraveWalletPreferExtension &&
          isMetaMaskInstalled))) &&
      showDefaultWalletBanner
  }, [defaultEthereumWallet, defaultSolanaWallet, isMetaMaskInstalled, showDefaultWalletBanner])

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
      {showBanner &&
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
    showBanner,
    needsBackup,
    onClickSettings,
    onClickShowMore,
    onDismissBackupWarning,
    onDismissDefaultWalletBanner,
    onOpenWalletSettings,
    onSelectTab,
    onShowBackup,
    showBackupWarning,
    showMore
  ])

  // render
  return (
    <StyledWrapper onClick={onClickHideMore}>
      <Switch>
        {/* Portfolio */}
        <Route path={WalletRoutes.AddAssetModal} exact>{/* Show portfolio overview in background */}
          {nav}
          <PortfolioOverview />
        </Route>

        <Route path={WalletRoutes.PortfolioAsset} exact>
          <PortfolioAsset />
        </Route>

        <Route path={WalletRoutes.Portfolio}>
          {nav}
          <PortfolioOverview />
        </Route>

        {/* Accounts */}
        <Route path={WalletRoutes.AddAccountModal}>{/* Show accounts overview in background */}
          {nav}
          <Accounts />
        </Route>

        <Route path={WalletRoutes.Account}>
          <Account
            toggleNav={toggleNav}
            onUpdateAccountName={onUpdateAccountName}
            onDoneViewingPrivateKey={onDoneViewingPrivateKey}
            onViewPrivateKey={onViewPrivateKey}
            goBack={goBack}
          />
        </Route>

        <Route path={WalletRoutes.Accounts}>
          {nav}
          <Accounts />
        </Route>

        <Redirect to={sessionRoute || WalletRoutes.Portfolio} />

      </Switch>

      {/* modals */}
      <Switch>
        <Route path={WalletRoutes.AddAssetModal} exact>
          <EditVisibleAssetsModal
            onClose={hideVisibleAssetsModal}
          />
        </Route>

        <Route path={WalletRoutes.AddAccountModal}>
          <AddAccountModal />
        </Route>
      </Switch>
    </StyledWrapper>
  )
}

export default CryptoView
