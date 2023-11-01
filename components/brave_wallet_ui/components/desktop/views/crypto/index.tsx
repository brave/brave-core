// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  Route,
  useHistory,
  useLocation,
  Switch,
  Redirect
} from 'react-router-dom'
import { useSelector } from 'react-redux'

// actions
import { AccountsTabState } from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// style
import { StyledWrapper } from './style'

// components
import { WalletBanner } from '../../wallet-banner/index'
import {
  EditVisibleAssetsModal //
} from '../../popup-modals/edit-visible-assets-modal/index'
import { PortfolioOverview } from '../portfolio/portfolio-overview'
import { PortfolioAsset } from '../portfolio/portfolio-asset'
import { PortfolioNftAsset } from '../portfolio/portfolio-nft-asset'
import { MarketView } from '../market'
import { Accounts } from '../accounts/accounts'
import { Account } from '../accounts/account'
import { AddAccountModal } from '../../popup-modals/add-account-modal/add-account-modal'
import { ConfirmPasswordModal } from '../../popup-modals/confirm-password-modal/confirm-password-modal'
import { AccountSettingsModal } from '../../popup-modals/account-settings-modal/account-settings-modal'
import TransactionsScreen from '../../../../page/screens/transactions/transactions-screen'
import { LocalIpfsNodeScreen } from '../../local-ipfs-node/local-ipfs-node'
import { InspectNftsScreen } from '../../inspect-nfts/inspect-nfts'
import { Column } from '../../../shared/style'
import {
  useSafeWalletSelector,
  useSafeUISelector
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors, UISelectors } from '../../../../common/selectors'
import {
  WalletPageWrapper //
} from '../../wallet-page-wrapper/wallet-page-wrapper'
import {
  PortfolioOverviewHeader //
} from '../../card-headers/portfolio-overview-header'
import { PageTitleHeader } from '../../card-headers/page-title-header'

export interface Props {
  onOpenWalletSettings: () => void
  needsBackup: boolean
  defaultEthereumWallet: BraveWallet.DefaultWallet
  defaultSolanaWallet: BraveWallet.DefaultWallet
  isMetaMaskInstalled: boolean
  sessionRoute: string | undefined
}

export const CryptoView = (props: Props) => {
  const {
    onOpenWalletSettings,
    defaultEthereumWallet,
    defaultSolanaWallet,
    needsBackup,
    isMetaMaskInstalled,
    sessionRoute
  } = props

  // accounts tab state
  const accountToRemove = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.accountToRemove
  )
  const showAccountModal = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.showAccountModal
  )
  const selectedAccount = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.selectedAccount
  )

  const isNftPinningFeatureEnabled = useSafeWalletSelector(
    WalletSelectors.isNftPinningFeatureEnabled
  )
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  // const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] =
    React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] =
    React.useState<boolean>(needsBackup)

  // routing
  const history = useHistory()
  const location = useLocation()

  // methods
  const onShowBackup = () => {
    if (isPanel) {
      chrome.tabs.create(
        {
          url: `chrome://wallet${WalletRoutes.Backup}`
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message
            )
          }
        }
      )
      return
    }
    history.push(WalletRoutes.Backup)
  }

  const onShowVisibleAssetsModal = React.useCallback((showModal: boolean) => {
    if (showModal) {
      history.push(WalletRoutes.AddAssetModal)
    } else {
      history.push(WalletRoutes.PortfolioAssets)
    }
  }, [])

  const onDismissBackupWarning = React.useCallback(() => {
    setShowBackupWarning(false)
  }, [])

  const onDismissDefaultWalletBanner = React.useCallback(() => {
    setShowDefaultWalletBanner(false)
  }, [])

  const hideVisibleAssetsModal = React.useCallback(
    () => onShowVisibleAssetsModal(false),
    [onShowVisibleAssetsModal]
  )

  const onClose = React.useCallback(() => {
    history.push(WalletRoutes.PortfolioNFTs)
  }, [])

  const onBack = React.useCallback(() => {
    if (location.key) {
      history.goBack()
    } else {
      history.push(WalletRoutes.PortfolioNFTs)
    }
  }, [location.key])

  const showBanner = React.useMemo((): boolean => {
    return (
      (defaultEthereumWallet !== BraveWallet.DefaultWallet.BraveWallet ||
        defaultSolanaWallet !== BraveWallet.DefaultWallet.BraveWallet) &&
      (defaultEthereumWallet !==
        BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
        defaultSolanaWallet !==
          BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
        (defaultEthereumWallet ===
          BraveWallet.DefaultWallet.BraveWalletPreferExtension &&
          isMetaMaskInstalled)) &&
      showDefaultWalletBanner
    )
  }, [
    defaultEthereumWallet,
    defaultSolanaWallet,
    isMetaMaskInstalled,
    showDefaultWalletBanner
  ])

  // memos
  const banners = React.useMemo(
    () => (
      <>
        {showBanner && (
          <WalletBanner
            onDismiss={onDismissDefaultWalletBanner}
            onClick={onOpenWalletSettings}
            bannerType='warning'
            buttonText={getLocale('braveWalletWalletPopupSettings')}
            description={getLocale('braveWalletDefaultWalletBanner')}
          />
        )}
        {needsBackup && showBackupWarning && (
          <WalletBanner
            onDismiss={onDismissBackupWarning}
            onClick={onShowBackup}
            bannerType='danger'
            buttonText={getLocale('braveWalletBackupButton')}
            description={getLocale('braveWalletBackupWarningText')}
          />
        )}
      </>
    ),
    [
      showBanner,
      needsBackup,
      onDismissBackupWarning,
      onDismissDefaultWalletBanner,
      onOpenWalletSettings,
      onShowBackup,
      showBackupWarning
    ]
  )

  // render
  return (
    <>
      <Switch>
        {/* Portfolio */}
        <Route
          path={WalletRoutes.AddAssetModal}
          exact
        >
          {/* Show portfolio overview in background */}
          <WalletPageWrapper
            wrapContentInBox={true}
            noCardPadding={true}
            cardHeader={<PortfolioOverviewHeader />}
            useDarkBackground={isPanel}
          >
            <StyledWrapper>
              <Column
                fullWidth={true}
                padding='20px 20px 0px 20px'
              >
                {banners}
              </Column>
              <PortfolioOverview />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.PortfolioNFTAsset}
          exact
        >
          <PortfolioNftAsset />
        </Route>

        <Route
          path={WalletRoutes.PortfolioAsset}
          exact
        >
          <PortfolioAsset />
        </Route>

        <Route path={WalletRoutes.Portfolio}>
          <WalletPageWrapper
            wrapContentInBox={true}
            noCardPadding={true}
            cardHeader={<PortfolioOverviewHeader />}
            useDarkBackground={isPanel}
          >
            <StyledWrapper>
              <Column
                fullWidth={true}
                padding='20px 20px 0px 20px'
              >
                {banners}
              </Column>
              <PortfolioOverview />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        {/* Accounts */}
        <Route path={WalletRoutes.AddAccountModal}>
          {/* Show accounts overview in background */}
          <WalletPageWrapper wrapContentInBox={true}>
            <StyledWrapper>
              {banners}
              <Accounts />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        <Route path={WalletRoutes.Account}>
          <Account />
        </Route>

        <Route path={WalletRoutes.Accounts}>
          <Accounts />
        </Route>

        {/* Market */}
        <Route
          path={WalletRoutes.Market}
          exact={true}
        >
          <WalletPageWrapper
            wrapContentInBox
            cardHeader={
              <PageTitleHeader title={getLocale('braveWalletTopNavMarket')} />
            }
          >
            <StyledWrapper>
              {banners}
              <MarketView />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        <Route
          path={WalletRoutes.MarketSub}
          exact={true}
        >
          <WalletPageWrapper wrapContentInBox={true}>
            <StyledWrapper>
              {banners}
              <PortfolioAsset isShowingMarketData={true} />
            </StyledWrapper>
          </WalletPageWrapper>
        </Route>

        {/* Transactions */}
        <Route
          path={WalletRoutes.Activity}
          exact={true}
        >
          <TransactionsScreen />
        </Route>

        {/* NFT Pinning onboarding page */}
        <Route
          path={WalletRoutes.LocalIpfsNode}
          exact={true}
          render={(props) =>
            isNftPinningFeatureEnabled ? (
              <WalletPageWrapper
                noPadding={true}
                hideNav={true}
                hideHeader={true}
              >
                <StyledWrapper>
                  <LocalIpfsNodeScreen
                    onClose={onClose}
                    {...props}
                  />
                </StyledWrapper>
              </WalletPageWrapper>
            ) : (
              <Redirect to={WalletRoutes.PortfolioAssets} />
            )
          }
        />

        {/* NFT Pinning inspect pinnable page */}
        <Route
          path={WalletRoutes.InspectNfts}
          exact={true}
          render={(props) =>
            isNftPinningFeatureEnabled ? (
              <WalletPageWrapper
                noPadding={true}
                hideNav={true}
                hideHeader={true}
              >
                <StyledWrapper>
                  <InspectNftsScreen
                    onClose={onClose}
                    onBack={onBack}
                    {...props}
                  />
                </StyledWrapper>
              </WalletPageWrapper>
            ) : (
              <Redirect to={WalletRoutes.PortfolioAssets} />
            )
          }
        />

        <Redirect to={sessionRoute || WalletRoutes.PortfolioAssets} />
      </Switch>

      {/* modals */}
      <Switch>
        <Route
          path={WalletRoutes.AddAssetModal}
          exact
        >
          <EditVisibleAssetsModal onClose={hideVisibleAssetsModal} />
        </Route>

        <Route path={WalletRoutes.AddAccountModal}>
          <AddAccountModal />
        </Route>
      </Switch>

      {accountToRemove !== undefined && <ConfirmPasswordModal />}

      {showAccountModal && selectedAccount && <AccountSettingsModal />}
    </>
  )
}

export default CryptoView
