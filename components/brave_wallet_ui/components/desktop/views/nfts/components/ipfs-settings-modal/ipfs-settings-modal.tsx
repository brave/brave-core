// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

// types
import { BraveWallet } from '../../../../../../constants/types'

// selectors
import { PageSelectors } from '../../../../../../page/selectors'
import { useUnsafePageSelector } from '../../../../../../common/hooks/use-safe-selector'

// components
import PopupModal from '../../../../popup-modals'
import {
  Column,
  HorizontalSpace,
  Row,
  ScrollableColumn,
  VerticalSpace
} from '../../../../../shared/style'

// styles
import {
  CollapseIcon,
  ContentWrapper,
  Divider,
  IpfsIcon,
  IpfsIconWrapper,
  ProgressCount,
  SectionHeader,
  SettingDescription,
  SettingHeader,
  NftGrid,
  GridItem
} from './ipfs-settings-modal.style'
import { NftIcon } from '../../../../../shared/nft-icon/nft-icon'
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

interface Props {
  nfts: BraveWallet.BlockchainToken[]
  onClose: () => void
}

export const IpfsSettingsModal = ({ nfts, onClose }: Props) => {
  const [autoSaveNftData, setAutoSaveNftData] = React.useState<boolean>(true)

  // hooks
  const nftsPinningStatus = useUnsafePageSelector(
    PageSelectors.nftsPinningStatus
  )
  console.log(nftsPinningStatus)

  return (
    <PopupModal
      onClose={onClose}
      title='IPFS Settings'
      width='500px'
      borderRadius={16}
    >
      <ContentWrapper fullWidth={true} alignItems='flex-start'>
        <Row alignItems='flex-start'>
          <IpfsIconWrapper>
            <IpfsIcon />
          </IpfsIconWrapper>
          <HorizontalSpace style={{ flexShrink: 0 }} space='16px' />
          <Column>
            <SettingHeader>Auto-save NFT data</SettingHeader>
            <SettingDescription>
              Enable IPFS in Brave to automatically back up your NFT data. NFT
              data will be fetched and securely stored to decentralized storage.
            </SettingDescription>
          </Column>
          <HorizontalSpace style={{ flexShrink: 0 }} space='16px' />
          <Toggle
            checked={autoSaveNftData}
            onChange={() => setAutoSaveNftData((current) => !current)}
            size='small'
          />
        </Row>
        <VerticalSpace space='24px' />
        <ScrollableColumn fullWidth maxHeight='500px'>
          <Row
            justifyContent='space-between'
            margin='12px 8px'
            marginBottom={12}
          >
            <SectionHeader>Pinned</SectionHeader>
            <Row justifyContent='flex-end'>
              <ProgressCount>3 NFTs</ProgressCount>
              <HorizontalSpace space='16px' />
              <CollapseIcon isCollapsed={true} name='carat-down' />
            </Row>
          </Row>
          <Divider />
          <NftGrid>
            {nfts.map((nft) => (
              <GridItem key={getAssetIdKey(nft)}>
                <NftIcon icon={nft.logo} responsive={true} />
              </GridItem>
            ))}
          </NftGrid>
          <Row
            justifyContent='space-between'
            margin='12px 8px'
            marginBottom={12}
          >
            <SectionHeader>Pinned</SectionHeader>
            <Row justifyContent='flex-end'>
              <ProgressCount>3 NFTs</ProgressCount>
              <HorizontalSpace space='16px' />
              <CollapseIcon isCollapsed={true} name='carat-down' />
            </Row>
          </Row>
          <Divider />
          <NftGrid>
            {nfts.map((nft) => (
              <GridItem key={getAssetIdKey(nft)}>
                <NftIcon icon={nft.logo} responsive={true} />
              </GridItem>
            ))}
          </NftGrid>
        </ScrollableColumn>
      </ContentWrapper>
    </PopupModal>
  )
}
