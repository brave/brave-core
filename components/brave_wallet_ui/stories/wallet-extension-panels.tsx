// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'

import './locale'
import {
  BraveWallet,
  SerializableTransactionInfo,
  UIState,
  WalletState
} from '../constants/types'

// Components
import {
  ConnectWithSite //
} from '../components/extension/connect-with-site-panel/connect-with-site-panel'
import {
  ConfirmTransactionPanel //
} from '../components/extension/confirm-transaction-panel/confirm-transaction-panel'
import { WelcomePanel } from '../components/extension/welcome-panel/index'
import {
  AddSuggestedTokenPanel //
} from '../components/extension/add-suggested-token-panel/index'
import {
  DecryptRequestPanel //
} from '../components/extension/encryption-key-panel/index'

import {
  StyledExtensionWrapperLonger,
  StyledExtensionWrapper,
  StyledWelcomPanel
} from './style'
import WalletPanelStory from './wrappers/wallet-panel-story-wrapper'

// mocks
import {
  mockTransactionInfo,
  mockedErc20ApprovalTransaction
} from './mock-data/mock-transaction-info'
import { mockAccounts } from './mock-data/mock-wallet-accounts'
import { mockDecryptRequest } from './mock-data/mock-encryption-key-payload'
import { mockOriginInfo } from './mock-data/mock-origin-info'
import { createMockStore } from '../utils/test-utils'
import { deserializeTransaction } from '../utils/model-serialization-utils'
import { WalletApiDataOverrides } from '../constants/testing_types'

export default {
  title: 'Wallet/Extension/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
}

const mockEthAccountId = (
  address: string
): { fromAddress: string; fromAccountId: BraveWallet.AccountId } => {
  return {
    fromAddress: address,
    fromAccountId: {
      coin: BraveWallet.CoinType.ETH,
      keyringId: BraveWallet.KeyringId.kDefault,
      kind: BraveWallet.AccountKind.kDerived,
      address: address,
      accountIndex: 0,
      uniqueKey: `${address}_id`
    }
  }
}

const transactionDummyData: SerializableTransactionInfo[][] = [
  [
    {
      chainId: '',
      ...mockEthAccountId('0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf5AAAA'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: 'ETHEREUM ACCOUNT 2',
            value: '0xb1a2bc2ec50000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 3,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: Date.now() * 1000 - 1000 * 60 * 5 * 1000 },
      submittedTime: { microseconds: Date.now() * 1000 - 1000 * 60 * 5 },
      confirmedTime: { microseconds: Date.now() * 1000 - 1000 * 60 * 5 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    },
    {
      chainId: '',
      ...mockEthAccountId('0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: '0xcd3a3f8e0e4bdc174c9e2e63b4c22e15a7f7f92a',
            value: '0xb1a2bc2ec50000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 3,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    },
    {
      chainId: '',
      ...mockEthAccountId('0x7843981e0b96135073b26043ea24c950d4ec385b'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
            value: '0xb1a2bc2ec90000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 4,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    },
    {
      chainId: '',
      ...mockEthAccountId('0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: '0xcd3a3f8e0e4bdc174c9e2e63b4c22e15a7f7f92a',
            value: '0xb1a2bc2ec90000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 2,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    },
    {
      chainId: '',
      ...mockEthAccountId('0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: '0xcd3a3f8e0e4bdc174c9e2e63b4c22e15a7f7f92a',
            value: '0xb1a2bc2ec90000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 1,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    }
  ],
  [
    {
      chainId: '',
      ...mockEthAccountId('0x73A29A1da97149722eB09c526E4eAd698895bDCf'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: '0xcd3a3f8e0e4bdc174c9e2e63b4c22e15a7f7f92a',
            value: '0xb1a2bc2ec90000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 0,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    },
    {
      chainId: '',
      ...mockEthAccountId('0x73A29A1da97149722eB09c526E4eAd698895bDCf'),
      id: '13cf4882-d3c0-44cd-a8c2-aca1fcf85c4a',
      txDataUnion: {
        ethTxData1559: {
          baseData: {
            data: Array.from(new Uint8Array(24)),
            gasLimit: '0xfde8',
            gasPrice: '0x20000000000',
            nonce: '0x1',
            to: '0xcd3a3f8e0e4bdc174c9e2e63b4c22e15a7f7f92a',
            value: '0xb1a2bc2ec90000',
            signOnly: false,
            signedTransaction: undefined
          },
          chainId: '',
          maxFeePerGas: '',
          maxPriorityFeePerGas: '',
          gasEstimation: undefined
        },
        ethTxData: undefined,
        solanaTxData: undefined,
        filTxData: undefined
      },
      txHash:
        '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 5,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      effectiveRecipient: '',
      isRetriable: false
    }
  ]
]

const originInfo = mockOriginInfo

const store = createMockStore(
  {
    walletStateOverride: {},
    uiStateOverride: {
      selectedPendingTransactionId: mockTransactionInfo.id
    }
  },
  {
    transactionInfos: [
      ...transactionDummyData[0].map((tx) => deserializeTransaction(tx)),
      ...transactionDummyData[1].map((tx) => deserializeTransaction(tx))
    ]
  }
)

const transactionList = [
  mockTransactionInfo,
  ...transactionDummyData[0],
  ...transactionDummyData[1]
]

const mockCustomStoreState: Partial<WalletState> = {
  activeOrigin: originInfo
}

const mockCustomUiState: Partial<UIState> = {
  selectedPendingTransactionId: mockTransactionInfo.id,
  transactionProviderErrorRegistry: {}
}

const mockApiData: WalletApiDataOverrides = {
  transactionInfos: transactionList.map(deserializeTransaction)
}

export const _ConfirmTransaction = () => {
  return (
    <Provider
      store={createMockStore(
        {
          walletStateOverride: mockCustomStoreState,
          uiStateOverride: mockCustomUiState
        },
        mockApiData
      )}
    >
      <StyledExtensionWrapperLonger>
        <ConfirmTransactionPanel />
      </StyledExtensionWrapperLonger>
    </Provider>
  )
}

_ConfirmTransaction.story = {
  name: 'Confirm Transaction'
}

export const _ConfirmErcApproveTransaction = () => {
  return (
    <StyledExtensionWrapperLonger>
      <Provider
        store={createMockStore(
          {
            uiStateOverride: {
              selectedPendingTransactionId: mockedErc20ApprovalTransaction.id
            }
          },
          {
            ...mockApiData,
            transactionInfos: [
              deserializeTransaction(mockedErc20ApprovalTransaction),
              ...(mockApiData?.transactionInfos ?? [])
            ]
          }
        )}
      >
        <ConfirmTransactionPanel />
      </Provider>
    </StyledExtensionWrapperLonger>
  )
}

_ConfirmErcApproveTransaction.story = {
  name: 'Confirm ERC20 Approval Transaction'
}

export const _ReadEncryptedMessage = () => {
  return (
    <StyledExtensionWrapperLonger>
      <DecryptRequestPanel payload={mockDecryptRequest} />
    </StyledExtensionWrapperLonger>
  )
}

_ReadEncryptedMessage.story = {
  name: 'Read Encrypted Message'
}

export const _ConnectWithSite = () => {
  return (
    <Provider store={store}>
      <StyledExtensionWrapperLonger>
        <ConnectWithSite
          originInfo={originInfo}
          accountsToConnect={mockAccounts}
        />
      </StyledExtensionWrapperLonger>
    </Provider>
  )
}

_ConnectWithSite.story = {
  name: 'Connect With Site'
}

export const _SetupWallet = () => {
  return (
    <StyledWelcomPanel>
      <WelcomePanel />
    </StyledWelcomPanel>
  )
}

_SetupWallet.story = {
  name: 'Setup New Wallet'
}

export const _AddSuggestedToken = () => {
  return (
    <StyledExtensionWrapper>
      <WalletPanelStory>
        <AddSuggestedTokenPanel />
      </WalletPanelStory>
    </StyledExtensionWrapper>
  )
}

_AddSuggestedToken.story = {
  name: 'Add Suggested Token'
}
