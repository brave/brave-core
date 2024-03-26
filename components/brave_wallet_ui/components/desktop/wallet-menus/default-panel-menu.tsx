// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import Toggle from '@brave/leo/react/toggle'

// Types
import {
  AccountPageTabs,
  BraveWallet,
  WalletRoutes
} from '../../../constants/types'

// Constants
import {
  LOCAL_STORAGE_KEYS //
} from '../../../common/constants/local-storage-keys'

// Options
import { CreateAccountOptions } from '../../../options/nav-options'

// utils
import { getLocale } from '../../../../common/locale'
import {
  useGetSelectedChainQuery,
  useLockWalletMutation
} from '../../../common/slices/api.slice'
import { openWalletSettings } from '../../../utils/routes-utils'
import { useSyncedLocalStorage } from '../../../common/hooks/use_local_storage'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
  ToggleRow
} from './wellet-menus.style'
import { VerticalDivider, VerticalSpace, Row } from '../../shared/style'

export interface Props {
  onClosePopup?: () => void
  yPosition?: number
}

export const DefaultPanelMenu = (props: Props) => {
  const { onClosePopup, yPosition } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // local-storage
  const [hidePortfolioBalances, setHidePortfolioBalances] =
    useSyncedLocalStorage(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES, false)
  const [hidePortfolioNFTsTab, setHidePortfolioNFTsTab] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
    false
  )
  const [hidePortfolioGraph, setHidePortfolioGraph] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
    false
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

    const route =
      selectedNetwork.coin === BraveWallet.CoinType.ETH ? 'ethereum' : 'solana'

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
    chrome.tabs.create(
      {
        url: 'https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet'
      },
      () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      }
    )
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
    if (route === WalletRoutes.AddHardwareAccountModalStart) {
      chrome.tabs.create({ url: `chrome://wallet${route}` }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      })
      return
    }
    history.push(route)
  }

  const onClickBackup = () => {
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
  }

  const showPortfolioSettings =
    walletLocation === WalletRoutes.PortfolioNFTs ||
    walletLocation === WalletRoutes.PortfolioAssets

  return (
    <StyledWrapper yPosition={yPosition}>
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
          {getLocale('braveWalletBackupButton')}
        </PopupButtonText>
      </PopupButton>

      {selectedNetwork && selectedNetwork.coin !== BraveWallet.CoinType.FIL && (
        <PopupButton onClick={onClickConnectedSites}>
          <ButtonIcon name='link-normal' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupConnectedSites')}
          </PopupButtonText>
        </PopupButton>
      )}

      <PopupButton onClick={onClickSettings}>
        <ButtonIcon name='settings' />
        <PopupButtonText>
          {getLocale('braveWalletWalletPopupSettings')}
        </PopupButtonText>
      </PopupButton>

      <VerticalDivider />
      <VerticalSpace space='8px' />

      {showPortfolioSettings && (
        <>
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

          <VerticalDivider />
          <VerticalSpace space='8px' />
        </>
      )}

      {walletLocation === WalletRoutes.Accounts && (
        <>
          {CreateAccountOptions.map((option) => (
            <PopupButton
              key={option.name}
              onClick={() => onClickRoute(option.route)}
              minWidth={240}
            >
              <ButtonIcon name={option.icon} />
              <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
            </PopupButton>
          ))}
          <VerticalDivider />
          <VerticalSpace space='8px' />
        </>
      )}

      <PopupButton onClick={onClickHelpCenter}>
        <ButtonIcon name='help-outline' />
        <PopupButtonText>{getLocale('braveWalletHelpCenter')}</PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}

export default DefaultPanelMenu
