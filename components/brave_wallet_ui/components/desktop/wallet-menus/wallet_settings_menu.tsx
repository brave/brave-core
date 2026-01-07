// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import Toggle from '@brave/leo/react/toggle'

// Page API Proxy
import getWalletPageApiProxy from '../../../page/wallet_page_api_proxy'

// Selectors
import { UISelectors } from '../../../common/selectors'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Types
import {
  AccountPageTabs,
  BraveWallet,
  WalletRoutes,
} from '../../../constants/types'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  useGetSelectedChainQuery,
  useLockWalletMutation,
} from '../../../common/slices/api.slice'
import { openWalletSettings } from '../../../utils/routes-utils'
import { useSyncedLocalStorage } from '../../../common/hooks/use_local_storage'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
  ToggleRow,
  SectionLabel,
} from './wellet-menus.style'
import { VerticalDivider, Row, Column } from '../../shared/style'

export interface Props {
  onClosePopup?: () => void
  yPosition?: number
}

const HELP_CENTER_URL =
  'https://support.brave.app/hc/categories/360001062531-Wallet'

export const WalletSettingsMenu = (props: Props) => {
  const { onClosePopup, yPosition } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)
  const isMobileOrPanel = isMobile || isPanel

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // local-storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)
  const [hidePortfolioNFTsTab, setHidePortfolioNFTsTab] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
    false,
  )
  const [hidePortfolioGraph, setHidePortfolioGraph] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
    true,
  )

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // mutations
  const [lockWallet] = useLockWalletMutation()

  // methods
  const onClickConnectedSites = React.useCallback(() => {
    if (!selectedNetwork) {
      return
    }

    let route: string
    switch (selectedNetwork.coin) {
      case BraveWallet.CoinType.ETH:
        route = 'ethereum'
        break
      case BraveWallet.CoinType.SOL:
        route = 'solana'
        break
      case BraveWallet.CoinType.ADA:
        route = 'cardano'
        break
      default:
        throw new Error('Coin not supported')
    }

    chrome.tabs.create({ url: `brave://settings/content/${route}` }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
    if (onClosePopup) {
      onClosePopup()
    }
  }, [selectedNetwork, onClosePopup])

  const onClickHelpCenter = React.useCallback(() => {
    if (chrome.tabs !== undefined) {
      chrome.tabs.create(
        {
          url: HELP_CENTER_URL,
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message,
            )
          }
        },
      )
    } else {
      // Tabs.create is desktop specific. Using window.open for mobile
      window.open(HELP_CENTER_URL, '_blank', 'noopener noreferrer')
    }

    if (onClosePopup) {
      onClosePopup()
    }
  }, [onClosePopup])

  const onClickSettings = React.useCallback(() => {
    openWalletSettings()
    if (onClosePopup) {
      onClosePopup()
    }
  }, [onClosePopup])

  // Methods
  const onToggleHideGraph = React.useCallback(() => {
    setHidePortfolioGraph((prev) => !prev)
  }, [setHidePortfolioGraph])

  const onToggleHideBalances = React.useCallback(() => {
    setHidePortfolioBalances((prev) => !prev)
  }, [setHidePortfolioBalances])

  const onToggleHideNFTsTab = React.useCallback(() => {
    if (walletLocation.includes(WalletRoutes.PortfolioNFTs)) {
      history.push(WalletRoutes.PortfolioAssets)
    }
    setHidePortfolioNFTsTab((prev) => !prev)
  }, [history, setHidePortfolioNFTsTab, walletLocation])

  const onClickRoute = (route: WalletRoutes | AccountPageTabs) => {
    if (route === WalletRoutes.AddHardwareAccountModalStart && isPanel) {
      chrome.tabs.create({ url: `chrome://wallet${route}` }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message,
          )
        }
      })
      return
    }
    history.push(route)
  }

  const onClickBackup = React.useCallback(() => {
    if (isMobile) {
      getWalletPageApiProxy().pageHandler.showWalletBackupUI()
      return
    }

    if (isPanel) {
      chrome.tabs.create(
        {
          url: `chrome://wallet${WalletRoutes.Backup}`,
        },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + chrome.runtime.lastError.message,
            )
          }
        },
      )
      return
    }
    history.push(WalletRoutes.Backup)
  }, [isMobile, isPanel, history])

  // Memos
  const accountSettingsOptions = React.useMemo(() => {
    if (isMobile) {
      return CreateAccountOptions.filter(
        (option) => option.name !== 'braveWalletConnectHardwareWallet',
      )
    }
    return CreateAccountOptions
  }, [isMobile])

  return (
    <StyledWrapper
      yPosition={yPosition}
      padding='0px'
    >
      <Column
        fullWidth={true}
        padding='8px 8px 0px 8px'
      >
        <PopupButton
          onClick={async () => {
            await lockWallet()
          }}
        >
          <ButtonIcon name='lock' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupLock')}
          </PopupButtonText>
        </PopupButton>

        <PopupButton onClick={onClickBackup}>
          <ButtonIcon name='safe' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupBackup')}
          </PopupButtonText>
        </PopupButton>

        {(selectedNetwork?.coin === BraveWallet.CoinType.ETH
          || selectedNetwork?.coin === BraveWallet.CoinType.SOL)
          && !isMobile && (
            <PopupButton onClick={onClickConnectedSites}>
              <ButtonIcon name='link-normal' />
              <PopupButtonText>
                {getLocale('braveWalletWalletPopupConnectedSites')}
              </PopupButtonText>
            </PopupButton>
          )}

        {!isMobile && (
          <PopupButton onClick={onClickSettings}>
            <ButtonIcon name='settings' />
            <PopupButtonText>
              {getLocale('braveWalletWalletPopupSettings')}
            </PopupButtonText>
          </PopupButton>
        )}
      </Column>

      {(walletLocation === WalletRoutes.PortfolioNFTs
        || walletLocation === WalletRoutes.PortfolioAssets
        || walletLocation === WalletRoutes.PortfolioActivity) && (
        <>
          <SectionLabel justifyContent='flex-start'>
            {getLocale('braveWalletPortfolioSettings')}
          </SectionLabel>
          <Column
            fullWidth={true}
            padding='8px 8px 0px 8px'
          >
            <ToggleRow onClick={onToggleHideBalances}>
              <Row>
                <ButtonIcon name='eye-on' />
                <PopupButtonText>
                  {getLocale('braveWalletWalletPopupHideBalances')}
                </PopupButtonText>
                <Toggle
                  checked={!hidePortfolioBalances}
                  onChange={onToggleHideBalances}
                  size='small'
                />
              </Row>
            </ToggleRow>

            <ToggleRow onClick={onToggleHideGraph}>
              <Row>
                <ButtonIcon name='graph' />
                <PopupButtonText>
                  {getLocale('braveWalletWalletPopupShowGraph')}
                </PopupButtonText>
              </Row>
              <Toggle
                checked={!hidePortfolioGraph}
                onChange={onToggleHideGraph}
                size='small'
              />
            </ToggleRow>

            <ToggleRow onClick={onToggleHideNFTsTab}>
              <Row>
                <ButtonIcon name='nft' />
                <PopupButtonText>
                  {getLocale('braveWalletWalletNFTsTab')}
                </PopupButtonText>
              </Row>
              <Toggle
                checked={!hidePortfolioNFTsTab}
                onChange={onToggleHideNFTsTab}
                size='small'
              />
            </ToggleRow>
          </Column>
        </>
      )}

      {walletLocation === WalletRoutes.Accounts && isMobileOrPanel && (
        <>
          <SectionLabel justifyContent='flex-start'>
            {getLocale('braveWalletAccountSettings')}
          </SectionLabel>
          <Column
            fullWidth={true}
            padding='8px 8px 0px 8px'
          >
            {accountSettingsOptions.map((option) => (
              <PopupButton
                key={option.name}
                onClick={() => onClickRoute(option.route)}
                minWidth={240}
              >
                <ButtonIcon name={option.icon} />
                <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
              </PopupButton>
            ))}
          </Column>
        </>
      )}

      <VerticalDivider />

      <Column
        fullWidth={true}
        padding='8px 8px 0px 8px'
      >
        <PopupButton onClick={onClickHelpCenter}>
          <ButtonIcon name='help-outline' />
          <PopupButtonText>
            {getLocale('braveWalletHelpCenter')}
          </PopupButtonText>
        </PopupButton>
      </Column>
    </StyledWrapper>
  )
}
