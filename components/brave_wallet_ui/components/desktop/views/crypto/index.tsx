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
import { useDispatch, useSelector } from 'react-redux'

// actions
import { AccountsTabState } from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import {
  BraveWallet,
  WalletRoutes
} from '../../../../constants/types'

// style
import { StyledWrapper } from './style'

// hooks
import { useBalanceUpdater } from '../../../../common/hooks/use-balance-updater'

// components
import { WalletBanner, EditVisibleAssetsModal } from '../../'
import { PortfolioOverview } from '../portfolio/portfolio-overview'
import { PortfolioAsset } from '../portfolio/portfolio-asset'
import { MarketView } from '../market'
import { Accounts } from '../accounts/accounts'
import { Account } from '../accounts/account'
import { AddAccountModal } from '../../popup-modals/add-account-modal/add-account-modal'
import { ConfirmPasswordModal } from '../../popup-modals/confirm-password-modal/confirm-password-modal'
import { AccountSettingsModal } from '../../popup-modals/account-settings-modal/account-settings-modal'
import TransactionsScreen from '../../../../page/screens/transactions/transactions-screen'
import { LocalIpfsNodeScreen } from '../../local-ipfs-node/local-ipfs-node'
import { InspectNftsScreen } from '../../inspect-nfts/inspect-nfts'
import { WalletPageActions } from '../../../../page/actions'
import { useNftPin } from '../../../../common/hooks/nft-pin'
import {
  BannerWrapper,
  Column
} from '../../../shared/style'
import { NftIpfsBanner } from '../../nft-ipfs-banner/nft-ipfs-banner'
import { useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

export interface Props {
  onOpenWalletSettings: () => void
  needsBackup: boolean
  defaultEthereumWallet: BraveWallet.DefaultWallet
  defaultSolanaWallet: BraveWallet.DefaultWallet
  isMetaMaskInstalled: boolean
  sessionRoute: string | undefined
}

const CryptoView = (props: Props) => {
  const {
    onOpenWalletSettings,
    defaultEthereumWallet,
    defaultSolanaWallet,
    needsBackup,
    isMetaMaskInstalled,
    sessionRoute
  } = props

  // hooks
  useBalanceUpdater()
  const { isIpfsBannerVisible, nonFungibleTokens, onToggleShowIpfsBanner } = useNftPin()

  // accounts tab state
  const accountToRemove = useSelector(({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab.accountToRemove)
  const showAccountModal = useSelector(({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab.showAccountModal)
  const selectedAccount = useSelector(({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab.selectedAccount)

  const isNftPinningFeatureEnabled = useSafeWalletSelector(WalletSelectors.isNftPinningFeatureEnabled)

  const dispatch = useDispatch()

  // state
  // const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)

  // routing
  const history = useHistory()
  const location = useLocation()

  // methods
  const onShowBackup = React.useCallback(() => {
    history.push(WalletRoutes.Backup)
  }, [])

  const onShowVisibleAssetsModal = React.useCallback((showModal: boolean) => {
    if (showModal) {
      history.push(`${WalletRoutes.AddAssetModal}`)
    } else {
      history.push(`${WalletRoutes.PortfolioAsset}`)
    }
  }, [])

  const onDismissBackupWarning = React.useCallback(() => {
    setShowBackupWarning(false)
  }, [])

  const onDismissDefaultWalletBanner = React.useCallback(() => {
    setShowDefaultWalletBanner(false)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
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
      (defaultEthereumWallet !== BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
        defaultSolanaWallet !== BraveWallet.DefaultWallet.BraveWalletPreferExtension ||
        (defaultEthereumWallet === BraveWallet.DefaultWallet.BraveWalletPreferExtension &&
          isMetaMaskInstalled))) &&
      showDefaultWalletBanner
  }, [defaultEthereumWallet, defaultSolanaWallet, isMetaMaskInstalled, showDefaultWalletBanner])

  // memos
  const banners = React.useMemo(() => (
    <>
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
    showBanner,
    needsBackup,
    onDismissBackupWarning,
    onDismissDefaultWalletBanner,
    onOpenWalletSettings,
    onShowBackup,
    showBackupWarning
  ])

  const ipfsBanner = React.useMemo(() => (
    <>
      {isNftPinningFeatureEnabled && isIpfsBannerVisible && nonFungibleTokens.length > 0 &&
        <BannerWrapper>
          <NftIpfsBanner onDismiss={onToggleShowIpfsBanner} />
        </BannerWrapper>
      }
    </>
  ), [isNftPinningFeatureEnabled, isIpfsBannerVisible, nonFungibleTokens])

  // effects
  React.useEffect(() => {
    dispatch(WalletPageActions.getLocalIpfsNodeStatus())
    dispatch(WalletPageActions.getIsAutoPinEnabled())
  }, [])

  // render
  return (
    <StyledWrapper>
      <Switch>
        {/* Portfolio */}
        <Route path={WalletRoutes.AddAssetModal} exact>{/* Show portfolio overview in background */}
          <Column
            fullWidth={true}
            padding='20px 20px 0px 20px'
          >
            {banners}
            {ipfsBanner}
          </Column>
          <PortfolioOverview
            onToggleShowIpfsBanner={onToggleShowIpfsBanner}
          />
        </Route>

        <Route path={WalletRoutes.PortfolioAsset} exact>
          <PortfolioAsset />
        </Route>

        <Route path={WalletRoutes.Portfolio}>
          <Column
            fullWidth={true}
            padding='20px 20px 0px 20px'
          >
            {banners}
            {ipfsBanner}
          </Column>
          <PortfolioOverview
            onToggleShowIpfsBanner={onToggleShowIpfsBanner}
          />
        </Route>

        {/* Accounts */}
        <Route path={WalletRoutes.AddAccountModal}>{/* Show accounts overview in background */}
          {banners}
          <Accounts />
        </Route>

        <Route path={WalletRoutes.Account}>
          <Account
            goBack={goBack}
          />
        </Route>

        <Route path={WalletRoutes.Accounts}>
          {banners}
          <Accounts />
        </Route>

        {/* Market */}
        <Route path={WalletRoutes.Market} exact={true}>
          {banners}
          <MarketView />
        </Route>

        <Route path={WalletRoutes.MarketSub} exact={true}>
          {banners}
          <PortfolioAsset
            isShowingMarketData={true}
          />
        </Route>

        {/* Transactions */}
        <Route path={WalletRoutes.Activity} exact={true}>
          {banners}
          <TransactionsScreen />
        </Route>

        {/* NFT Pinning onboarding page */}
        <Route
          path={WalletRoutes.LocalIpfsNode}
          exact={true}
          render={(props) => isNftPinningFeatureEnabled
            ? <LocalIpfsNodeScreen onClose={onClose} {...props} />
            : <Redirect to={WalletRoutes.PortfolioAssets} />
          }
        />

        {/* NFT Pinning inspect pinnable page */}
        <Route
          path={WalletRoutes.InspectNfts}
          exact={true}
          render={(props) => isNftPinningFeatureEnabled
            ? <InspectNftsScreen onClose={onClose} onBack={onBack} {...props} />
            : <Redirect to={WalletRoutes.PortfolioAssets} />
          }
        />

        <Redirect to={sessionRoute || WalletRoutes.PortfolioAssets} />

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

      {accountToRemove !== undefined &&
        <ConfirmPasswordModal />
      }

      {showAccountModal && selectedAccount &&
        <AccountSettingsModal />
      }
    </StyledWrapper>
  )
}

export default CryptoView
