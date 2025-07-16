// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'
import Button from '@brave/leo/react/button'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import {
  useAcknowledgePendingAddChainRequestMutation,
  useAcknowledgeSwitchChainRequestMutation,
  useGetNetworkForAccountOnActiveOriginQuery,
  useGetNetworkQuery,
} from '../../../common/slices/api.slice'
import {
  useSelectedETHAccountQuery, //
} from '../../../common/slices/api.slice.extra'

// Components
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'
import { CreateNetworkIcon } from '../../shared/create-network-icon'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'
import { NetworkInfo } from '../network_info/network_info'

// Styled Components
import {
  StyledWrapper,
  HeaderText,
  Title,
  Description,
  OriginText,
  DetailsButton,
  LearnMoreButton,
  FavIcon,
  NetworkInfoBox,
  NetworkInfoLabel,
  NetworkInfoText,
  DividerWrapper,
  ArrowIconWrapper,
  ArrowIcon,
} from './allow_add_change_network_panel.style'
import { Column, Row, VerticalDivider } from '../../shared/style'

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
        'https://support.brave.com'
        + '/hc/en-us/articles/4415497656461-Brave-Wallet-FAQ',
    })
    .catch((e) => {
      console.error(e)
    })
}

export function AllowAddChangeNetworkPanel(props: Props) {
  const { addChainRequest, switchChainRequest } = props

  // state
  const [showNetworkDetails, setShowNetworkDetails] = React.useState(false)

  // queries
  const { data: selectedETHAccount } = useSelectedETHAccountQuery()
  const { data: selectedNetwork } = useGetNetworkForAccountOnActiveOriginQuery({
    accountId: selectedETHAccount?.accountId,
  })

  const { data: switchChainRequestNetwork } = useGetNetworkQuery(
    switchChainRequest
      ? {
          chainId: switchChainRequest.chainId,
          // Passed ETH here since AllowAddChangeNetworkPanel
          // is only used for EVM networks
          // and switchChainRequest doesn't return coinType.
          coin: BraveWallet.CoinType.ETH,
        }
      : skipToken,
  )
  const addChainRequestNetwork = addChainRequest?.networkInfo
  const networkDetails = switchChainRequest
    ? switchChainRequestNetwork
    : addChainRequestNetwork

  // mutations
  const [acknowledgeSwitchChainRequest] =
    useAcknowledgeSwitchChainRequestMutation()

  // computed from props
  const originInfo = switchChainRequest
    ? switchChainRequest.originInfo
    : addChainRequest.originInfo

  // mutations
  const [acknowledgePendingAddChainRequest] =
    useAcknowledgePendingAddChainRequestMutation()

  const onApproveAddNetwork = async () => {
    if (!addChainRequest) {
      return
    }
    await acknowledgePendingAddChainRequest({
      chainId: addChainRequest.networkInfo.chainId,
      isApproved: true,
    }).unwrap()
  }

  const onApproveChangeNetwork = async () => {
    if (!switchChainRequest) {
      return
    }
    await acknowledgeSwitchChainRequest({
      requestId: switchChainRequest.requestId,
      isApproved: true,
    }).unwrap()
  }

  const onCancelAddNetwork = async () => {
    if (!addChainRequest) {
      return
    }
    await acknowledgePendingAddChainRequest({
      chainId: addChainRequest.networkInfo.chainId,
      isApproved: false,
    }).unwrap()
  }

  const onCancelChangeNetwork = async () => {
    if (!switchChainRequest) {
      return
    }
    await acknowledgeSwitchChainRequest({
      requestId: switchChainRequest.requestId,
      isApproved: false,
    }).unwrap()
  }

  // render
  return (
    <StyledWrapper
      justifyContent='space-between'
      width='100%'
      height='100%'
    >
      <Column width='100%'>
        <Row padding='18px'>
          <HeaderText textColor='primary'>
            {switchChainRequest
              ? getLocale('braveWalletAllowChangeNetworkButton')
              : getLocale('braveWalletAddNetwork')}
          </HeaderText>
        </Row>
        <Column
          padding='16px 24px 24px 24px'
          gap='8px'
        >
          <FavIcon
            src={`chrome://favicon2?size=64&pageUrl=${encodeURIComponent(
              originInfo.originSpec,
            )}`}
          />
          <OriginText textColor='tertiary'>
            <CreateSiteOrigin
              originSpec={originInfo.originSpec}
              eTldPlusOne={originInfo.eTldPlusOne}
            />
          </OriginText>
          <Title textColor='primary'>
            {switchChainRequest
              ? getLocale('braveWalletAllowChangeNetworkTitle')
              : getLocale('braveWalletAllowAddNetworkTitle')}
          </Title>
          <Description textColor='tertiary'>
            {switchChainRequest
              ? getLocale('braveWalletAllowChangeNetworkDescription')
              : getLocale('braveWalletAllowAddNetworkDescription')}{' '}
            {addChainRequest && (
              <LearnMoreButton onClick={onLearnMore}>
                {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
              </LearnMoreButton>
            )}
          </Description>
        </Column>
        <Column
          padding='0 24px'
          width='100%'
          gap='16px'
        >
          {switchChainRequest && (
            <NetworkInfoBox
              alignItems='flex-start'
              justifyContent='flex-start'
              padding='16px'
              gap='22px'
              width='100%'
            >
              <Column
                alignItems='flex-start'
                justifyContent='flex-start'
              >
                <NetworkInfoLabel textColor='secondary'>
                  {getLocale('braveWalletFrom')}:
                </NetworkInfoLabel>
                <Row
                  gap='8px'
                  justifyContent='flex-start'
                >
                  <CreateNetworkIcon
                    size='huge'
                    network={selectedNetwork}
                  />
                  <Column
                    alignItems='flex-start'
                    justifyContent='flex-start'
                  >
                    <NetworkInfoText
                      textColor='primary'
                      textAlign='left'
                    >
                      {selectedNetwork?.chainName ?? ''}
                    </NetworkInfoText>
                    <NetworkInfoLabel
                      textColor='secondary'
                      textAlign='left'
                    >
                      {selectedNetwork?.rpcEndpoints[
                        selectedNetwork?.activeRpcEndpointIndex
                      ]?.url ?? ''}
                    </NetworkInfoLabel>
                  </Column>
                </Row>
              </Column>
              <DividerWrapper>
                <ArrowIconWrapper>
                  <ArrowIcon />
                </ArrowIconWrapper>
                <VerticalDivider />
              </DividerWrapper>
              <Column
                alignItems='flex-start'
                justifyContent='flex-start'
                width='100%'
              >
                <NetworkInfoLabel textColor='secondary'>
                  {getLocale('braveWalletSwapTo')}:
                </NetworkInfoLabel>
                <Row
                  gap='8px'
                  justifyContent='flex-start'
                >
                  <CreateNetworkIcon
                    size='huge'
                    network={switchChainRequestNetwork}
                  />
                  <Column
                    alignItems='flex-start'
                    justifyContent='flex-start'
                    width='100%'
                  >
                    <Row
                      justifyContent='space-between'
                      alignItems='center'
                    >
                      <NetworkInfoText
                        textColor='primary'
                        textAlign='left'
                      >
                        {switchChainRequestNetwork?.chainName ?? ''}
                      </NetworkInfoText>
                      <DetailsButton
                        onClick={() => setShowNetworkDetails(true)}
                      >
                        {getLocale('braveWalletDetails')}
                      </DetailsButton>
                    </Row>
                    <NetworkInfoLabel
                      textColor='secondary'
                      textAlign='left'
                    >
                      {switchChainRequestNetwork?.rpcEndpoints[
                        switchChainRequestNetwork?.activeRpcEndpointIndex
                      ]?.url || ''}
                    </NetworkInfoLabel>
                  </Column>
                </Row>
              </Column>
            </NetworkInfoBox>
          )}
          {addChainRequest && (
            <NetworkInfoBox
              alignItems='flex-start'
              justifyContent='flex-start'
              padding='16px'
              gap='16px'
              width='100%'
            >
              <Column
                alignItems='flex-start'
                justifyContent='flex-start'
              >
                <NetworkInfoLabel textColor='secondary'>
                  {getLocale('braveWalletAllowAddNetworkName')}
                </NetworkInfoLabel>
                <NetworkInfoText textColor='primary'>
                  {addChainRequestNetwork?.chainName ?? ''}
                </NetworkInfoText>
              </Column>
              <VerticalDivider />
              <Column
                alignItems='flex-start'
                justifyContent='flex-start'
              >
                <NetworkInfoLabel textColor='secondary'>
                  {getLocale('braveWalletAllowAddNetworkUrl')}
                </NetworkInfoLabel>
                <NetworkInfoText textColor='primary'>
                  {addChainRequestNetwork?.rpcEndpoints[
                    addChainRequestNetwork?.activeRpcEndpointIndex
                  ]?.url || ''}
                </NetworkInfoText>
              </Column>
            </NetworkInfoBox>
          )}
          {addChainRequest && (
            <Button
              kind='plain'
              size='medium'
              onClick={() => setShowNetworkDetails(true)}
            >
              {getLocale('braveWalletAllowAddNetworkDetailsButton')}
            </Button>
          )}
        </Column>
      </Column>
      <Row
        padding='16px'
        gap='8px'
      >
        <Button
          kind='outline'
          size='medium'
          onClick={addChainRequest ? onCancelAddNetwork : onCancelChangeNetwork}
        >
          {getLocale('braveWalletButtonCancel')}
        </Button>
        <Button
          kind='filled'
          size='medium'
          onClick={
            addChainRequest ? onApproveAddNetwork : onApproveChangeNetwork
          }
        >
          {switchChainRequest
            ? getLocale('braveWalletAllowChangeNetworkButton')
            : getLocale('braveWalletAllowAddNetworkButton')}
        </Button>
      </Row>
      <BottomSheet
        isOpen={showNetworkDetails}
        onClose={() => setShowNetworkDetails(false)}
        title={networkDetails?.chainName ?? ''}
      >
        {networkDetails && (
          <Column
            padding='24px 16px 16px 16px'
            width='100%'
          >
            <NetworkInfo network={networkDetails} />
          </Column>
        )}
      </BottomSheet>
    </StyledWrapper>
  )
}

export default AllowAddChangeNetworkPanel
