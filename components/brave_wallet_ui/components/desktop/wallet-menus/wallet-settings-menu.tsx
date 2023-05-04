// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import Checkbox from '@brave/leo/react/checkbox'

// Selectors
import {
  useSafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import {
  WalletSelectors
} from '../../../common/selectors'

// Types
import { BraveWallet } from '../../../constants/types'
import {
  LOCAL_STORAGE_KEYS
} from '../../../common/constants/local-storage-keys'

// actions
import { WalletActions } from '../../../common/actions'

// utils
import { getLocale } from '../../../../common/locale'
import { useGetSelectedChainQuery } from '../../../common/slices/api.slice'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
  CheckBoxRow,
  SectionTitle
} from './wellet-menus.style'
import {
  VerticalDivider,
  VerticalSpace,
  Row
} from '../../shared/style'

export interface Props {
  onClickViewOnBlockExplorer?: () => void
  onClickBackup?: () => void
  onClosePopup?: () => void
  yPosition?: number
  isPanel?: boolean
}

export const WalletSettingsMenu = (props: Props) => {
  const {
    onClickViewOnBlockExplorer,
    onClickBackup,
    onClosePopup,
    yPosition,
    isPanel
  } = props

  // redux
  const dispatch = useDispatch()

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // redux
  const hidePortfolioGraph =
    useSafeWalletSelector(WalletSelectors.hidePortfolioGraph)
  const hidePortfolioBalances =
    useSafeWalletSelector(WalletSelectors.hidePortfolioBalances)

  // methods
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

  const lockWallet = React.useCallback(() => {
    dispatch(WalletActions.lockWallet())
  }, [])

  const onClickConnectedSites = React.useCallback(() => {
    if (!selectedNetwork) {
      return
    }

    const route = selectedNetwork.coin === BraveWallet.CoinType.ETH
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

      {onClickBackup &&
        <PopupButton onClick={onClickBackup}>
          <ButtonIcon name='safe' />
          <PopupButtonText>
            {getLocale('braveWalletBackupButton')}
          </PopupButtonText>
        </PopupButton>
      }

      {
        selectedNetwork &&
        selectedNetwork.coin !== BraveWallet.CoinType.FIL &&
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

      {onClickViewOnBlockExplorer &&
        <PopupButton onClick={onClickViewOnBlockExplorer}>
          <ButtonIcon name='launch' />
          <PopupButtonText>
            {getLocale('braveWalletTransactionExplorer')}
          </PopupButtonText>
        </PopupButton>
      }

      <PopupButton onClick={onClickHelpCenter}>
        <ButtonIcon name='help-outline' />
        <PopupButtonText>
          {getLocale('braveWalletHelpCenter')}
        </PopupButtonText>
      </PopupButton>

      {/* We can remove this prop once PanelV2 is ready. */}
      {!isPanel &&
        <>
          <VerticalDivider />
          <VerticalSpace space='14px' />

          <Row
            justifyContent='flex-start'
            padding='0px 0px 0px 8px'
            marginBottom={8}
          >
            <SectionTitle
              textSize='12px'
              textColor='text02'
              textAlign='left'
              isBold={true}
            >
              {getLocale(
                'braveWalletWalletPopupPortfolioCustomizations'
              )}
            </SectionTitle>
          </Row>

          <CheckBoxRow onClick={onToggleHideBalances}>
            <ButtonIcon name='eye-off' />
            <PopupButtonText>
              {getLocale('braveWalletWalletPopupHideBalances')}
            </PopupButtonText>
            <Checkbox
              checked={hidePortfolioBalances}
              onChanged={onToggleHideBalances}
              size='normal'
            />
          </CheckBoxRow>

          <CheckBoxRow onClick={onToggleHideGraph}>
            <Row>
              {/* This graph icon needs to be updated to the
              one in figma once it is added to leo. */}
              <ButtonIcon name='graph' />
              <PopupButtonText>
                {getLocale('braveWalletWalletPopupShowGraph')}
              </PopupButtonText>
            </Row>
            <Checkbox
              checked={!hidePortfolioGraph}
              onChanged={onToggleHideGraph}
              size='normal'
            />
          </CheckBoxRow>
        </>
      }

    </StyledWrapper>
  )
}

export default WalletSettingsMenu
