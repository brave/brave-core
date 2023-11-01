// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'

import './locale'
import {
  AppsListType,
  BraveWallet,
  PanelTypes,
  SerializableTransactionInfo,
  UIState,
  WalletState
} from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import { filterAppList } from '../utils/filter-app-list'

// Components
import {
  ConnectWithSite //
} from '../components/extension/connect-with-site-panel/connect-with-site-panel'
import {
  ConfirmTransactionPanel //
} from '../components/extension/confirm-transaction-panel/confirm-transaction-panel'
import { ConnectedPanel } from '../components/extension/connected-panel/index'
import { Panel } from '../components/extension/panel/index'
import { WelcomePanel } from '../components/extension/welcome-panel/index'
import { SignPanel } from '../components/extension/sign-panel/index'
import {
  AllowAddChangeNetworkPanel //
} from '../components/extension/allow-add-change-network-panel/index'
import {
  ConnectHardwareWalletPanel //
} from '../components/extension/connect-hardware-wallet-panel/index'
import {
  SitePermissions //
} from '../components/extension/site-permissions-panel/index'
import {
  AddSuggestedTokenPanel //
} from '../components/extension/add-suggested-token-panel/index'
import {
  TransactionsPanel //
} from '../components/extension/transactions-panel/index'
import {
  TransactionDetailPanel //
} from '../components/extension/transaction-detail-panel/index'
import { AssetsPanel } from '../components/extension/assets-panel/index'
import {
  DecryptRequestPanel,
  ProvidePubKeyPanel //
} from '../components/extension/encryption-key-panel/index'

import { AppList } from '../components/shared/app-list/index'
import { SelectAccountWithHeader } from '../components/buy-send-swap/select-account-with-header'
import { CreateAccountTab } from '../components/buy-send-swap/create-account'
import { SelectNetworkWithHeader } from '../components/buy-send-swap/select-network-with-header'
import LockPanel from '../components/extension/lock-panel'
import {
  StyledExtensionWrapperLonger,
  StyledExtensionWrapper,
  ScrollContainer,
  SelectContainer,
  StyledWelcomPanel,
  StyledCreateAccountPanel
} from './style'
import { mockNetworks } from './mock-data/mock-networks'
import { PANEL_TITLES } from '../options/panel-titles'
import { LibContext } from '../common/context/lib.context'
import WalletPanelStory from './wrappers/wallet-panel-story-wrapper'

// mocks
import * as MockedLib from '../common/async/__mocks__/lib'
import {
  mockTransactionInfo,
  mockedErc20ApprovalTransaction
} from './mock-data/mock-transaction-info'
import {
  mockAccounts,
  mockedTransactionAccounts
} from './mock-data/mock-wallet-accounts'
import {
  mockEncryptionKeyRequest,
  mockDecryptRequest
} from './mock-data/mock-encryption-key-payload'
import { mockOriginInfo } from './mock-data/mock-origin-info'
import { mockNewAssetOptions } from './mock-data/mock-asset-options'
import { createMockStore } from '../utils/test-utils'
import { deserializeTransaction } from '../utils/model-serialization-utils'
import { WalletApiDataOverrides } from '../common/async/__mocks__/bridge'

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
      bitcoinAccountIndex: 0,
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
      effectiveRecipient: ''
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
      effectiveRecipient: ''
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
      effectiveRecipient: ''
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
      effectiveRecipient: ''
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
      effectiveRecipient: ''
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
      effectiveRecipient: ''
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
      effectiveRecipient: ''
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
  defaultCurrencies: { fiat: 'USD', crypto: 'ETH' },
  fullTokenList: mockNewAssetOptions,
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
  const getERC20Allowance = () => Promise.resolve('0x15ddf09c97b0000')

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
        <LibContext.Provider
          value={{
            ...(MockedLib as any),
            getERC20Allowance
          }}
        >
          <ConfirmTransactionPanel />
        </LibContext.Provider>
      </StyledExtensionWrapperLonger>
    </Provider>
  )
}

_ConfirmTransaction.story = {
  name: 'Confirm Transaction'
}

export const _ConfirmErcApproveTransaction = () => {
  const getERC20Allowance = () => Promise.resolve('0x15ddf09c97b0000')

  return (
    <StyledExtensionWrapperLonger>
      <Provider
        store={createMockStore(
          {
            walletStateOverride: {
              defaultCurrencies: { fiat: 'USD', crypto: 'ETH' },
              fullTokenList: mockNewAssetOptions
            },
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
        <LibContext.Provider
          value={{
            ...(MockedLib as any),
            getERC20Allowance
          }}
        >
          <ConfirmTransactionPanel />
        </LibContext.Provider>
      </Provider>
    </StyledExtensionWrapperLonger>
  )
}

_ConfirmErcApproveTransaction.story = {
  name: 'Confirm ERC20 Approval Transaction'
}

export const _AllowAddChangeNetwork = () => {
  const onApprove = () => {
    alert('Will Approve adding or chainging networks')
  }

  const onCancel = () => {
    alert('Canceled Adding Network')
  }

  return (
    <StyledExtensionWrapperLonger>
      <AllowAddChangeNetworkPanel
        originInfo={originInfo}
        panelType='change'
        onApproveAddNetwork={onApprove}
        onApproveChangeNetwork={onApprove}
        onCancel={onCancel}
        networkPayload={mockNetworks[0]}
      />
    </StyledExtensionWrapperLonger>
  )
}

_AllowAddChangeNetwork.story = {
  name: 'Allow Add or Change Network'
}

export const _SignData = () => {
  const onCancel = () => {
    alert('Canceled Signing Data')
  }

  const signMessageDataPayload: BraveWallet.SignMessageRequest[] = [
    {
      id: 0,
      accountId: mockEthAccountId('0x3f29A1da97149722eB09c526E4eAd698895b426')
        .fromAccountId,
      originInfo: mockOriginInfo,
      coin: BraveWallet.CoinType.ETH,
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      signData: {
        ethStandardSignData: undefined,
        ethSignTypedData: {
          message: 'Sign below to authenticate with CryptoKitties.',
          domain: '',
          domainHash: undefined,
          primaryHash: undefined,
          meta: undefined
        },
        ethSiweData: undefined,
        solanaSignData: undefined
      }
    }
  ]

  return (
    <StyledExtensionWrapperLonger>
      <SignPanel
        signMessageData={signMessageDataPayload}
        onCancel={onCancel}
        showWarning={true}
      />
    </StyledExtensionWrapperLonger>
  )
}

_SignData.story = {
  name: 'Sign Transaction'
}

export const _ProvideEncryptionKey = () => {
  const onProvide = () => {
    alert('Will Provide Encryption Key')
  }

  const onCancel = () => {
    alert('Will Cancel Providing Encryption Key')
  }

  return (
    <StyledExtensionWrapperLonger>
      <ProvidePubKeyPanel
        payload={mockEncryptionKeyRequest}
        onCancel={onCancel}
        onProvide={onProvide}
      />
    </StyledExtensionWrapperLonger>
  )
}

_ProvideEncryptionKey.story = {
  name: 'Provide Encryption Key'
}

export const _ReadEncryptedMessage = () => {
  const onAllow = () => {
    alert('Will Allow Reading Encrypted Message')
  }

  const onCancel = () => {
    alert('Will Not Allow Reading Encrypted Message')
  }

  return (
    <StyledExtensionWrapperLonger>
      <DecryptRequestPanel
        payload={mockDecryptRequest}
        onCancel={onCancel}
        onAllow={onAllow}
      />
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

export const _ConnectedPanel = (args: { locked: boolean }) => {
  const { locked } = args
  const [walletLocked, setWalletLocked] = React.useState<boolean>(locked)
  const [selectedPanel, setSelectedPanel] = React.useState<PanelTypes>('main')
  const [panelTitle, setPanelTitle] = React.useState<string>('main')
  const [selectedAccount, setSelectedAccount] =
    React.useState<BraveWallet.AccountInfo>(mockAccounts[0])
  const [favoriteApps, setFavoriteApps] = React.useState<BraveWallet.AppItem[]>(
    [AppsList()[0].appList[0]]
  )
  const [filteredAppsList, setFilteredAppsList] = React.useState<
    AppsListType[]
  >(AppsList())

  const selectedTransaction = transactionList[1][0]

  const onBack = () => {
    setSelectedPanel('main')
  }

  const onBackToTransactions = () => {
    navigateTo('activity')
  }

  const onSelectAccount = (account: BraveWallet.AccountInfo) => () => {
    setSelectedAccount(account)
    setSelectedPanel('main')
  }

  const getTitle = (path: PanelTypes) => {
    const title = PANEL_TITLES.find((title) => path === title.id)
    setPanelTitle(title ? title.title : '')
  }

  const navigateTo = (path: PanelTypes) => {
    if (path === 'expanded') {
      alert('This will expand to main wallet!')
    } else {
      setSelectedPanel(path)
    }
    getTitle(path)
  }

  const browseMore = () => {
    alert('Will expand to view more!')
  }

  const addToFavorites = (app: BraveWallet.AppItem) => {
    const newList = [...favoriteApps, app]
    setFavoriteApps(newList)
  }
  const removeFromFavorites = (app: BraveWallet.AppItem) => {
    const newList = favoriteApps.filter((fav) => fav.name !== app.name)
    setFavoriteApps(newList)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList(), setFilteredAppsList)
  }

  const unlockWallet = (_password: string) => {
    setWalletLocked(false)
  }

  const onRestore = () => {
    alert('Will navigate to full wallet restore page')
  }

  const onAddAccount = () => {
    console.log('Will Expand to the Accounts Page')
  }

  const onAddNetwork = () => {
    console.log('Will redirect user to network settings')
  }

  const onNoAccountForNetwork = (network: BraveWallet.NetworkInfo) => {
    console.log('Will expand to Create Account panel')
  }

  const onAddAsset = () => {
    alert('Will redirect to brave://wallet/crypto/portfolio/add-asset')
  }

  return (
    <WalletPanelStory>
      <StyledExtensionWrapper>
        {walletLocked ? (
          <LockPanel
            onSubmit={unlockWallet}
            onClickRestore={onRestore}
          />
        ) : (
          <>
            {selectedPanel === 'main' ? (
              <ConnectedPanel navAction={navigateTo} />
            ) : (
              <>
                {selectedPanel === 'accounts' && (
                  <SelectContainer>
                    <SelectAccountWithHeader
                      accounts={mockAccounts}
                      onBack={onBack}
                      onSelectAccount={onSelectAccount}
                      onAddAccount={onAddAccount}
                      hasAddButton={true}
                      selectedAccount={selectedAccount}
                    />
                  </SelectContainer>
                )}
                {selectedPanel === 'networks' && (
                  <SelectContainer>
                    <SelectNetworkWithHeader
                      onBack={onBack}
                      hasAddButton={true}
                      onAddNetwork={onAddNetwork}
                      onNoAccountForNetwork={onNoAccountForNetwork}
                    />
                  </SelectContainer>
                )}
                {selectedPanel === 'transactionDetails' &&
                  selectedTransaction && (
                    <SelectContainer>
                      <TransactionDetailPanel
                        transactionId={selectedTransaction.id}
                        onBack={onBackToTransactions}
                        visibleTokens={mockNewAssetOptions}
                      />
                    </SelectContainer>
                  )}
                {selectedPanel !== 'networks' &&
                  selectedPanel !== 'accounts' &&
                  selectedPanel !== 'transactionDetails' && (
                    <Panel
                      navAction={navigateTo}
                      title={panelTitle}
                      useSearch={selectedPanel === 'apps'}
                      searchAction={
                        selectedPanel === 'apps' ? filterList : undefined
                      }
                    >
                      <ScrollContainer>
                        {selectedPanel === 'apps' && (
                          <AppList
                            list={filteredAppsList}
                            favApps={favoriteApps}
                            addToFav={addToFavorites}
                            removeFromFav={removeFromFavorites}
                            action={browseMore}
                          />
                        )}

                        {selectedPanel === 'sitePermissions' && (
                          <SitePermissions />
                        )}

                        {/* Transactions */}
                        {selectedPanel === 'activity' && (
                          <TransactionsPanel
                            selectedNetwork={mockNetworks[0]}
                            selectedAccount={
                              mockedTransactionAccounts[0].accountId
                            }
                          />
                        )}
                        {selectedPanel === 'assets' && (
                          <AssetsPanel
                            selectedAccount={selectedAccount}
                            onAddAsset={onAddAsset}
                          />
                        )}
                      </ScrollContainer>
                    </Panel>
                  )}
              </>
            )}
          </>
        )}
      </StyledExtensionWrapper>
    </WalletPanelStory>
  )
}

_ConnectedPanel.args = {
  locked: false
}

_ConnectedPanel.story = {
  name: 'Connected With Site'
}

export const _SetupWallet = () => {
  const onSetup = () => {
    alert('Will navigate to full wallet onboarding page')
  }

  return (
    <StyledWelcomPanel>
      <WelcomePanel onSetup={onSetup} />
    </StyledWelcomPanel>
  )
}

_SetupWallet.story = {
  name: 'Setup New Wallet'
}

export const _ConnectHardwareWallet = () => {
  const onCancel = (account: BraveWallet.AccountInfo) => {
    // Doesn't do anything in storybook
  }

  const onClickInstructions = () => {
    // Open support link in new tab
    window.open(
      'https://support.brave.com/hc/en-us/articles/4409309138701',
      '_blank',
      'noreferrer'
    )
  }

  return (
    <StyledExtensionWrapper>
      <ConnectHardwareWalletPanel
        account={{ ...mockAccounts[0], name: 'Ledger 1' }}
        onCancel={onCancel}
        onClickInstructions={onClickInstructions}
        hardwareWalletCode={undefined}
      />
    </StyledExtensionWrapper>
  )
}

_ConnectHardwareWallet.story = {
  name: 'Connect Hardware Wallet'
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

export const _TransactionDetail = () => {
  const mockedFunction = () => {
    // Doesn't do anything in storybook
  }

  const tx = transactionList[mockedTransactionAccounts[0].address][0]
  return (
    <Provider
      store={createMockStore({ walletStateOverride: mockCustomStoreState })}
    >
      <StyledExtensionWrapper>
        <Panel
          navAction={mockedFunction}
          title={'Recent Transactions'}
          useSearch={false}
          searchAction={undefined}
        >
          <ScrollContainer>
            <TransactionDetailPanel
              onBack={mockedFunction}
              visibleTokens={mockNewAssetOptions}
              transactionId={tx.id}
            />
          </ScrollContainer>
        </Panel>
      </StyledExtensionWrapper>
    </Provider>
  )
}

_TransactionDetail.story = {
  name: 'Transactions Detail'
}

export const _RecentTransaction = () => {
  const navigateTo = () => {
    // Doesn't do anything in storybook
  }

  return (
    <Provider
      store={createMockStore({ walletStateOverride: mockCustomStoreState })}
    >
      <StyledExtensionWrapper>
        <Panel
          navAction={navigateTo}
          title={'Recent Transactions'}
          useSearch={false}
          searchAction={undefined}
        >
          <ScrollContainer>
            <TransactionsPanel
              selectedNetwork={mockNetworks[0]}
              selectedAccount={mockedTransactionAccounts[0].accountId}
            />
          </ScrollContainer>
        </Panel>
      </StyledExtensionWrapper>
    </Provider>
  )
}

_RecentTransaction.story = {
  name: 'Recent Transactions'
}

export const _CreateAccount = () => {
  return (
    <Provider
      store={createMockStore({ walletStateOverride: mockCustomStoreState })}
    >
      <StyledCreateAccountPanel>
        <CreateAccountTab
          network={mockNetworks[0]}
          onCancel={() => {}}
        />
      </StyledCreateAccountPanel>
    </Provider>
  )
}

_CreateAccount.story = {
  name: 'Create Account Tab'
}
