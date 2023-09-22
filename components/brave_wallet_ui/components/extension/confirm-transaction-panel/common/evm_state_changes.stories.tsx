// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'

// components
import {
  WalletPanelStory //
} from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import {
  ErcTokenApproval,
  EvmNativeAssetOrErc20TokenTransfer,
  NonFungibleErcTokenTransfer
} from './evm_state_changes'

// mocks
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'
import {
  mockApproveAllBoredApeNFTsEvent,
  mockApproveBoredApeNftTransferEvent,
  mockApproveUsdtEvent,
  mockReceiveMultiStandardTokenEvent,
  mockReceiveNftEvent,
  mockSendEthEvent,
  mockedReceiveDaiEvent
} from '../../../../common/constants/mocks'

export const _EvmErc20TokenTransfer = () => {
  return (
    <WalletPanelStory>
      <EvmNativeAssetOrErc20TokenTransfer
        transfer={mockedReceiveDaiEvent.rawInfo.data.erc20TransferData}
        network={mockEthMainnet}
      />
    </WalletPanelStory>
  )
}

export const _UnverifiedEvmErc20TokenTransfer = () => {
  return (
    <WalletPanelStory>
      <EvmNativeAssetOrErc20TokenTransfer
        transfer={{
          ...mockedReceiveDaiEvent.rawInfo.data.erc20TransferData,
          asset: {
            ...mockedReceiveDaiEvent.rawInfo.data.erc20TransferData.asset,
            verified: false,
            lists: [],
            address: 'UNKNOWN'
          }
        }}
        network={mockEthMainnet}
      />
    </WalletPanelStory>
  )
}

export const _EvmNativeAssetTransfer = () => {
  return (
    <WalletPanelStory>
      <EvmNativeAssetOrErc20TokenTransfer
        transfer={mockSendEthEvent.rawInfo.data.nativeAssetTransferData}
        network={mockEthMainnet}
      />
    </WalletPanelStory>
  )
}

export const _Erc721TokenTransfer = () => {
  return (
    <WalletPanelStory>
      <NonFungibleErcTokenTransfer
        network={mockEthMainnet}
        transfer={mockReceiveNftEvent.rawInfo.data.erc721TransferData}
      />
    </WalletPanelStory>
  )
}

export const _Erc1155TokenTransfer = () => {
  return (
    <WalletPanelStory>
      <NonFungibleErcTokenTransfer
        network={mockEthMainnet}
        transfer={
          mockReceiveMultiStandardTokenEvent.rawInfo.data.erc1155TransferData
        }
      />
    </WalletPanelStory>
  )
}

export const _Erc20TokenApproval = () => {
  return (
    <ErcTokenApproval
      approval={mockApproveUsdtEvent.rawInfo.data.erc20ApprovalData}
      network={mockEthMainnet}
      isApprovalForAll={false}
      isERC20
    />
  )
}

export const _Erc721TokenApproval = () => {
  return (
    <ErcTokenApproval
      approval={
        mockApproveBoredApeNftTransferEvent.rawInfo.data.erc721ApprovalData
      }
      network={mockEthMainnet}
      isApprovalForAll={false}
      isERC20={false}
    />
  )
}

export const _Erc721TokenApprovalForAll = () => {
  return (
    <ErcTokenApproval
      approval={
        mockApproveAllBoredApeNFTsEvent.rawInfo.data.erc721ApprovalForAllData
      }
      network={mockEthMainnet}
      isApprovalForAll={true}
      isERC20={false}
    />
  )
}

export default {}
