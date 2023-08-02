// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import { useDispatch } from 'react-redux'
import Toggle from '@brave/leo/react/toggle'

// Selectors
import {
  useSafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import {
  WalletSelectors
} from '../../../common/selectors'

// Types
import {
  CoinType,
  WalletRoutes
} from '../../../constants/types'

// Constants
import {
  LOCAL_STORAGE_KEYS
} from '../../../common/constants/local-storage-keys'

// actions
import { WalletActions } from '../../../common/actions'

// Options
import {
  CreateAccountOptions
} from '../../../options/nav-options'

// utils
import { getLocale } from '../../../../common/locale'
import { useGetSelectedChainQuery } from '../../../common/slices/api.slice'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
  ToggleRow
} from './wellet-menus.style'
import {
  VerticalDivider,
  VerticalSpace,
  Row
} from '../../shared/style'

export interface Props {
  onClosePopup?: () => void
  yPosition?: number
}

export const DefaultPanelMenu = (props: Props) => {
  const {
    onClosePopup,
    yPosition
  } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // redux
  const dispatch = useDispatch()

  // selectors
  const hidePortfolioGraph =
    useSafeWalletSelector(WalletSelectors.hidePortfolioGraph)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)
  const hidePortfolioNFTsTab =
    useSafeWalletSelector(WalletSelectors.hidePortfolioNFTsTab)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // methods
  const lockWallet = React.useCallback(() => {
    dispatch(WalletActions.lockWallet())
  }, [])

  const onClickConnectedSites = React.useCallback(() => {
    if (!selectedNetwork) {
      return
    }

    const route = selectedNetwork.coin === CoinType.ETH
      ? 'ethereum'
      : 'solana'

    chrome.tabs.create({ url: `brave://settings/content/${route}` }, () => {
      if (chrome.runtime.lastError) {
        console.error(
          'tabs.create failed: ' +
          chrome.runtime.lastError.message
        )
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
      }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: '
            + chrome.runtime.lastError.message
          )
        }
      })
    if (onClosePopup) {
      onClosePopup()
    }
  }, [onClosePopup])

  const onClickSettings = React.useCallback(() => {
    chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
      if (chrome.runtime.lastError) {
        console.error(
          'tabs.create failed: ' +
          chrome.runtime.lastError.message
        )
      }
    })
    if (onClosePopup) {
      onClosePopup()
    }
  }, [onClosePopup])

  // Methods
  const onToggleHideGraph = React.useCallback(() => {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN,
      hidePortfolioGraph
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioGraph(
          !hidePortfolioGraph
        ))
  }, [hidePortfolioGraph])

  const onToggleHideBalances = React.useCallback(() => {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
      hidePortfolioBalances
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioBalances(
          !hidePortfolioBalances
        ))
  }, [hidePortfolioBalances])

  const onToggleHideNFTsTab = React.useCallback(() => {
    if (walletLocation.includes(WalletRoutes.PortfolioNFTs)) {
      history.push(WalletRoutes.PortfolioAssets)
    }
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB,
      hidePortfolioNFTsTab
        ? 'false'
        : 'true'
    )
    dispatch(
      WalletActions
        .setHidePortfolioNFTsTab(
          !hidePortfolioNFTsTab
        ))
  }, [hidePortfolioNFTsTab, walletLocation])

  const showPortfolioSettings =
    walletLocation === WalletRoutes.PortfolioNFTs ||
    walletLocation === WalletRoutes.PortfolioAssets

  return (
    <StyledWrapper
      yPosition={yPosition}
    >

      <PopupButton onClick={lockWallet}>
        <ButtonIcon name='lock' />
        <PopupButtonText>
          {getLocale('braveWalletWalletPopupLock')}
        </PopupButtonText>
      </PopupButton>

      {
        selectedNetwork &&
        selectedNetwork.coin !== CoinType.FIL &&
        <PopupButton onClick={onClickConnectedSites}>
          <ButtonIcon name='link-normal' />
          <PopupButtonText>
            {getLocale('braveWalletWalletPopupConnectedSites')}
          </PopupButtonText>
        </PopupButton>
      }

      <PopupButton onClick={onClickSettings}>
        <ButtonIcon name='settings' />
        <PopupButtonText>
          {getLocale('braveWalletWalletPopupSettings')}
        </PopupButtonText>
      </PopupButton>

      <VerticalDivider />
      <VerticalSpace space='8px' />

      {showPortfolioSettings &&
        <>
          <ToggleRow onClick={onToggleHideBalances}>
            <Row>
              <ButtonIcon name='eye-on' />
              <PopupButtonText>
                {getLocale('braveWalletWalletPopupHideBalances')}
              </PopupButtonText>
              <Toggle
                checked={!hidePortfolioBalances}
                onChanged={onToggleHideBalances}
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
              onChanged={onToggleHideGraph}
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
              onChanged={onToggleHideNFTsTab}
              size='small'
            />
          </ToggleRow>

          <VerticalDivider />
          <VerticalSpace space='8px' />
        </>
      }

      {walletLocation === WalletRoutes.Accounts &&
        <>
          {CreateAccountOptions.map((option) =>
            <PopupButton
              key={option.name}
              onClick={
                () => history.push(option.route)
              }
              minWidth={240}
            >
              <ButtonIcon name={option.icon} />
              <PopupButtonText>
                {getLocale(option.name)}
              </PopupButtonText>
            </PopupButton>
          )}
          <VerticalDivider />
          <VerticalSpace space='8px' />
        </>
      }

      <PopupButton onClick={onClickHelpCenter}>
        <ButtonIcon name='help-outline' />
        <PopupButtonText>
          {getLocale('braveWalletHelpCenter')}
        </PopupButtonText>
      </PopupButton>

    </StyledWrapper>
  )
}

export default DefaultPanelMenu
