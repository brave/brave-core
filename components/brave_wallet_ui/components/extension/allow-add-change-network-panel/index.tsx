// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import {
  useAcknowledgePendingAddChainRequestMutation,
  useAcknowledgeSwitchChainRequestMutation,
  useGetNetworkQuery
} from '../../../common/slices/api.slice'

// Components
import { NavButton } from '../buttons/nav-button'
import { PanelTab } from '../panel-tab/index'
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'

// Styled Components
import {
  MessageBox,
  NetworkTitle,
  MessageBoxColumn,
  DetailsButton,
  ButtonRow,
  FavIcon,
  NetworkDetail,
  TabRow
} from './style'

import {
  StyledWrapper,
  CenterColumn,
  Description,
  PanelTitle,
  URLText
} from '../shared-panel-styles'

export type tabs = 'network' | 'details'

export type Props =
  | {
      addChainRequest: BraveWallet.AddChainRequest
      switchChainRequest?: never
    }
  | {
      switchChainRequest: BraveWallet.SwitchChainRequest
      addChainRequest?: never
    }

const onLearnMore = () => {
  chrome.tabs
    .create({
      url:
        'https://support.brave.com' +
        '/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ'
    })
    .catch((e) => {
      console.error(e)
    })
}

export function AllowAddChangeNetworkPanel(props: Props) {
  const { addChainRequest, switchChainRequest } = props

  // queries
  const { data: switchChainRequestNetwork } = useGetNetworkQuery(
    switchChainRequest
      ? {
          chainId: switchChainRequest.chainId,
          // Passed ETH here since AllowAddChangeNetworkPanel
          // is only used for EVM networks
          // and switchChainRequest doesn't return coinType.
          coin: BraveWallet.CoinType.ETH
        }
      : skipToken
  )

  const network = switchChainRequest
    ? switchChainRequestNetwork
    : addChainRequest.networkInfo

  // mutations
  const [acknowledgeSwitchChainRequest] =
    useAcknowledgeSwitchChainRequestMutation()

  // computed from props
  const rpcUrl =
    network?.rpcEndpoints[network?.activeRpcEndpointIndex]?.url || ''
  const blockUrl = network?.blockExplorerUrls.length
    ? network?.blockExplorerUrls[0]
    : ''

  const originInfo = switchChainRequest
    ? switchChainRequest.originInfo
    : addChainRequest.originInfo

  // mutations
  const [acknowledgePendingAddChainRequest] =
    useAcknowledgePendingAddChainRequestMutation()

  // state
  const [selectedTab, setSelectedTab] = React.useState<tabs>('network')

  // methods
  const onSelectTab = (tab: tabs) => () => {
    setSelectedTab(tab)
  }

  const onApproveAddNetwork = async () => {
    if (!addChainRequest) {
      return
    }
    await acknowledgePendingAddChainRequest({
      chainId: addChainRequest.networkInfo.chainId,
      isApproved: true
    }).unwrap()
  }

  const onApproveChangeNetwork = async () => {
    if (!switchChainRequest) {
      return
    }
    await acknowledgeSwitchChainRequest({
      requestId: switchChainRequest.requestId,
      isApproved: true
    }).unwrap()
  }

  const onCancelAddNetwork = async () => {
    if (!addChainRequest) {
      return
    }
    await acknowledgePendingAddChainRequest({
      chainId: addChainRequest.networkInfo.chainId,
      isApproved: false
    }).unwrap()
  }

  const onCancelChangeNetwork = async () => {
    if (!switchChainRequest) {
      return
    }
    await acknowledgeSwitchChainRequest({
      requestId: switchChainRequest.requestId,
      isApproved: false
    }).unwrap()
  }

  // render
  return (
    <StyledWrapper>
      <CenterColumn>
        <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`} />
        <URLText>
          <CreateSiteOrigin
            originSpec={originInfo.originSpec}
            eTldPlusOne={originInfo.eTldPlusOne}
          />
        </URLText>
        <PanelTitle>
          {switchChainRequest
            ? getLocale('braveWalletAllowChangeNetworkTitle')
            : getLocale('braveWalletAllowAddNetworkTitle')}
        </PanelTitle>
        <Description>
          {switchChainRequest
            ? getLocale('braveWalletAllowChangeNetworkDescription')
            : getLocale('braveWalletAllowAddNetworkDescription')}{' '}
          {addChainRequest && (
            <DetailsButton onClick={onLearnMore}>
              {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
            </DetailsButton>
          )}
        </Description>
        <TabRow>
          <PanelTab
            isSelected={selectedTab === 'network'}
            onSubmit={onSelectTab('network')}
            text={getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')}
          />
          <PanelTab
            isSelected={selectedTab === 'details'}
            onSubmit={onSelectTab('details')}
            text={getLocale('braveWalletDetails')}
          />
        </TabRow>
        <MessageBox>
          <MessageBoxColumn>
            <NetworkTitle>
              {getLocale('braveWalletAllowAddNetworkName')}
            </NetworkTitle>
            <NetworkDetail>{network?.chainName}</NetworkDetail>
          </MessageBoxColumn>
          <MessageBoxColumn>
            <NetworkTitle>
              {getLocale('braveWalletAllowAddNetworkUrl')}
            </NetworkTitle>
            <NetworkDetail>{rpcUrl}</NetworkDetail>
          </MessageBoxColumn>
          {selectedTab === 'details' && (
            <>
              <MessageBoxColumn>
                <NetworkTitle>{getLocale('braveWalletChainId')}</NetworkTitle>
                <NetworkDetail>{network?.chainId}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>
                  {getLocale('braveWalletAllowAddNetworkCurrencySymbol')}
                </NetworkTitle>
                <NetworkDetail>{network?.symbol}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>
                  {getLocale('braveWalletWatchListTokenDecimals')}
                </NetworkTitle>
                <NetworkDetail>{network?.decimals}</NetworkDetail>
              </MessageBoxColumn>
              <MessageBoxColumn>
                <NetworkTitle>
                  {getLocale('braveWalletAllowAddNetworkExplorer')}
                </NetworkTitle>
                <NetworkDetail>{blockUrl}</NetworkDetail>
              </MessageBoxColumn>
            </>
          )}
        </MessageBox>
      </CenterColumn>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={
            addChainRequest ? onCancelAddNetwork : onCancelChangeNetwork
          }
        />
        <NavButton
          buttonType='confirm'
          text={
            switchChainRequest
              ? getLocale('braveWalletAllowChangeNetworkButton')
              : getLocale('braveWalletAllowAddNetworkButton')
          }
          onSubmit={
            addChainRequest ? onApproveAddNetwork : onApproveChangeNetwork
          }
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default AllowAddChangeNetworkPanel
