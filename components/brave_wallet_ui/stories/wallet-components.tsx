// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { DesktopComponentWrapper, DesktopComponentWrapperRow } from './style'
import { SideNav, TopTabNav } from '../components/desktop'
import { NavTypes, TopTabNavTypes, BraveWallet } from '../constants/types'
import { NavOptions } from '../options/side-nav-options'
import { TopNavOptions } from '../options/top-nav-options'
import { ChartTimelineOptions } from '../options/chart-timeline-options'
import './locale'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { LoadingSkeleton } from '../components/shared'
import { ChartControlBar } from '../components/desktop/chart-control-bar/chart-control-bar'
import { WalletNav } from '../components/desktop/wallet-nav/wallet-nav'
import { NftIpfsBanner } from '../components/desktop/nft-ipfs-banner/nft-ipfs-banner'
import { LocalIpfsNodeScreen } from '../components/desktop/local-ipfs-node/local-ipfs-node'
import { InspectNftsScreen } from '../components/desktop/inspect-nfts/inspect-nfts'
import WalletPageStory from './wrappers/wallet-page-story-wrapper'
import { NftDetails } from '../nft/components/nft-details/nft-details'
import { mockNewAssetOptions } from './mock-data/mock-asset-options'
import { mockNFTMetadata } from './mock-data/mock-nft-metadata'
import { mockNetwork } from '../common/constants/mocks'
import { NftPinningStatus } from '../components/desktop/nft-pinning-status/nft-pinning-status'
import { NftsEmptyState } from '../components/desktop/views/nfts/components/nfts-empty-state/nfts-empty-state'
import { EnableNftDiscoveryModal } from '../components/desktop/popup-modals/enable-nft-discovery-modal/enable-nft-discovery-modal'

export default {
  title: 'Wallet/Desktop/Components',
  parameters: {
    layout: 'centered'
  }
}

export const _DesktopSideNav = () => {
  const [selectedButton, setSelectedButton] = React.useState<NavTypes>('crypto')

  const navigateTo = (path: NavTypes) => {
    setSelectedButton(path)
  }

  return (
    <DesktopComponentWrapper>
      <SideNav
        navList={NavOptions()}
        selectedButton={selectedButton}
        onSubmit={navigateTo}
      />
    </DesktopComponentWrapper>
  )
}

_DesktopSideNav.story = {
  name: 'Side Nav'
}

export const _DesktopTopTabNav = () => {
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')

  const onSelectTab = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  return (
    <DesktopComponentWrapperRow>
      <TopTabNav
        tabList={TopNavOptions()}
        selectedTab={selectedTab}
        onSelectTab={onSelectTab}
      />
    </DesktopComponentWrapperRow>
  )
}

_DesktopTopTabNav.story = {
  name: 'Top Tab Nav'
}

export const _LineChartControls = () => {
  const [selectedTimeline, setSelectedTimeline] = React.useState<BraveWallet.AssetPriceTimeframe>(BraveWallet.AssetPriceTimeframe.OneDay)

  return (
    <DesktopComponentWrapper>
      <ChartControlBar
        onSelectTimeframe={setSelectedTimeline}
        selectedTimeline={selectedTimeline}
        timelineOptions={ChartTimelineOptions}
      />
    </DesktopComponentWrapper>
  )
}

_LineChartControls.story = {
  name: 'Chart Controls'
}

export const _SweepstakesBanner = () => {
  return <SweepstakesBanner
    startDate={new Date(Date.now())}
    endDate={new Date(Date.now() + 1)}
  />
}

_SweepstakesBanner.story = {
  name: 'Sweepstakes Banner'
}

export const _LoadingSkeleton = () => {
  return (
  <div
    style={{
      width: '600px',
      display: 'flex',
      justifyContent: 'center',
      alignItems: 'center'
    }}>
    <LoadingSkeleton
      width={500}
      height={20}
      count={5}
    />
  </div>)
}

_LoadingSkeleton.story = {
  name: 'Loading Skeleton'
}

export const _BuySendSwapDeposit = () => {
  return (
    <WalletNav />
  )
}

_BuySendSwapDeposit.story = {
  name: 'Buy/Send/Swap/Deposit'
}

export const _NftIpfsBanner = () => {
  const [showBanner, setShowBanner] = React.useState(true)

  const onDismiss = React.useCallback(() => {
    setShowBanner(false)
  }, [])

  return (
    <div style={{ width: '855px' }}>
      {showBanner && <NftIpfsBanner onDismiss={onDismiss} />}
    </div>
  )
}

_NftIpfsBanner.story = {
  name: 'NFT IPFS Banner'
}

export const _LocalIpfsScreen = () => {
  const onClose = () => {
    console.log('close')
  }

  return (
    <LocalIpfsNodeScreen
      onClose={onClose}
    />
  )
}

_LocalIpfsScreen.story = {
  name: 'Run Local IPFS Node'
}

export const _InspectNftsScreen = () => {
  const onClose = () => {
    console.log('on close')
  }
  const onBack = () => {
    console.log('on back')
  }
  return (
    <WalletPageStory>
      <InspectNftsScreen
        onClose={onClose}
        onBack={onBack}
      />
    </WalletPageStory>
  )
}

_InspectNftsScreen.story = {
  name: 'Inspect NFTs Screen'
}

export const _NftDetails = () => {
  return (
    <NftDetails
      selectedAsset={{ ...mockNewAssetOptions[2], contractAddress: '0x7b539F7Cc90De458B60c2ab52eEf504B5de6D035', isErc721: true }}
      nftMetadata={mockNFTMetadata[0]}
      tokenNetwork={mockNetwork}
    />
  )
}

_NftDetails.story = {
  name: 'NFT Details'
}

export const _NftPinningStatus = () => {
  return (
    <div style={{ display: 'grid', gap: 10 }}>
      {/* uploading */}
      <NftPinningStatus
        pinningStatusCode={3}
      />

      {/* success */}
      <NftPinningStatus
        pinningStatusCode={2}
      />

      {/* failed */}
      <NftPinningStatus
        pinningStatusCode={4}
      />
    </div>
  )
}

_NftPinningStatus.story = {
  title: 'NFT Pinning Status'
}

export const _NftsEmptyState = () => {
  return (
    <NftsEmptyState
      onImportNft={() => console.log('On import NFT')}
    />
  )
}

export const _EnableNftDiscoveryModal = () => {
  return (
    <EnableNftDiscoveryModal
      onCancel={() => {}}
      onConfirm={() => {}}
    />
  )
}

_EnableNftDiscoveryModal.story = {
  title: 'Enable NFT Discovery Modal'
}
