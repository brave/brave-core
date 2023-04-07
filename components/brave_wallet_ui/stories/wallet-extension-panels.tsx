// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { configureStore } from '@reduxjs/toolkit'

import './locale'
import {
  AccountTransactions,
  AppsListType,
  BraveWallet,
  PanelTypes,
  SerializableTransactionInfo,
  WalletAccountType,
  WalletState
} from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import { filterAppList } from '../utils/filter-app-list'

// Components
import {
  ConnectWithSite,
  ConnectedPanel,
  Panel,
  WelcomePanel,
  SignPanel,
  AllowAddChangeNetworkPanel,
  ConfirmTransactionPanel,
  ConnectHardwareWalletPanel,
  SitePermissions,
  AddSuggestedTokenPanel,
  TransactionsPanel,
  TransactionDetailPanel,
  AssetsPanel,
  EncryptionKeyPanel
} from '../components/extension'
import { AppList } from '../components/shared'
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
import { PanelTitles } from '../options/panel-titles'
import { LibContext } from '../common/context/lib.context'
import WalletPanelStory from './wrappers/wallet-panel-story-wrapper'

// reducers & slices
import { walletApi } from '../common/slices/api.slice'
import { createWalletReducer } from '../common/slices/wallet.slice'
import { createPageReducer } from '../page/reducers/page_reducer'
import { createPanelReducer } from '../panel/reducers/panel_reducer'

// mocks
import * as MockedLib from '../common/async/__mocks__/lib'
import { mockedErc20ApprovalTransaction, mockTransactionInfo } from './mock-data/mock-transaction-info'
import { mockDefaultCurrencies } from './mock-data/mock-default-currencies'
import { mockTransactionSpotPrices } from './mock-data/current-price-data'
import { mockAccounts, mockedTransactionAccounts } from './mock-data/mock-wallet-accounts'
import { mockEncryptionKeyRequest, mockDecryptRequest } from './mock-data/mock-encryption-key-payload'
import { mockOriginInfo } from './mock-data/mock-origin-info'
import { mockAccountAssetOptions, mockNewAssetOptions } from './mock-data/mock-asset-options'
import { mockPanelState } from './mock-data/mock-panel-state'
import { mockPageState } from './mock-data/mock-page-state'
import { mockWalletState } from './mock-data/mock-wallet-state'
import { mockUserAccounts } from './mock-data/user-accounts'

export default {
  title: 'Wallet/Extension/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
}

const transactionDummyData: AccountTransactions = {
  [mockUserAccounts[0].id]: [
    {
      chainId: '',
      fromAddress: 'ETHEREUM ACCOUNT 1',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 3,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: (Date.now() * 1000) - 1000 * 60 * 5 * 1000 },
      submittedTime: { microseconds: (Date.now() * 1000) - 1000 * 60 * 5 },
      confirmedTime: { microseconds: (Date.now() * 1000) - 1000 * 60 * 5 },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
      chainId: '',
      fromAddress: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 3,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
      chainId: '',
      fromAddress: '0x7843981e0b96135073b26043ea24c950d4ec385b',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 4,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
      chainId: '',
      fromAddress: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 2,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
      chainId: '',
      fromAddress: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 1,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      groupId: undefined
    }
  ],
  [mockUserAccounts[1].id]: [
    {
      chainId: '',
      fromAddress: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 0,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
      chainId: '',
      fromAddress: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
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
      txHash: '0x55732e30af74a450cd438be2a02c765ea62cb4ec8dda5cb12ed8dc5d21ac15d3',
      txStatus: 5,
      txArgs: [],
      txParams: [],
      txType: 0,
      createdTime: { microseconds: 0 },
      submittedTime: { microseconds: 0 },
      confirmedTime: { microseconds: 0 },
      originInfo: mockOriginInfo,
      groupId: undefined
    }
  ]
}

const originInfo = mockOriginInfo

function createStoreWithCustomState (customWalletState: Partial<WalletState> = {}) {
  return configureStore({
    reducer: {
      wallet: createWalletReducer({
        ...mockWalletState,
        ...customWalletState
      }),
      page: createPageReducer(mockPageState),
      panel: createPanelReducer(mockPanelState),
      [walletApi.reducerPath]: walletApi.reducer

    },
    devTools: true,
    middleware: (getDefaultMiddleware) => getDefaultMiddleware().concat(walletApi.middleware)
  })
}

const store = createStoreWithCustomState()

const transactionList = {
  [mockedTransactionAccounts[0].address]: [
    ...transactionDummyData[1],
    ...transactionDummyData[2]
  ]
}

const mockCustomStoreState: Partial<WalletState> = {
  accounts: mockAccounts,
  selectedPendingTransaction: mockTransactionInfo,
  transactionSpotPrices: mockTransactionSpotPrices,
  defaultCurrencies: { fiat: 'USD', crypto: 'ETH' },
  fullTokenList: mockNewAssetOptions,
  activeOrigin: originInfo,
  transactions: transactionList
}

export const _ConfirmTransaction = () => {
  const onConfirmTransaction = () => alert('Confirmed Transaction')
  const onRejectTransaction = () => alert('Rejected Transaction')
  const getERC20Allowance = () => Promise.resolve('0x15ddf09c97b0000')

  return (
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledExtensionWrapperLonger>
        <LibContext.Provider value={{
          ...MockedLib as any,
          getERC20Allowance
        }}>
          <ConfirmTransactionPanel
            onConfirm={onConfirmTransaction}
            onReject={onRejectTransaction}
          />
        </LibContext.Provider>
      </StyledExtensionWrapperLonger>
    </Provider>
  )
}

_ConfirmTransaction.story = {
  name: 'Confirm Transaction'
}

export const _ConfirmErcApproveTransaction = () => {
  const onConfirmTransaction = () => alert('Confirmed Transaction')
  const onRejectTransaction = () => alert('Rejected Transaction')
  const getERC20Allowance = () => Promise.resolve('0x15ddf09c97b0000')

  return (
    <StyledExtensionWrapperLonger>
      <Provider store={createStoreWithCustomState({
        accounts: mockAccounts,
        selectedPendingTransaction: mockedErc20ApprovalTransaction,
        transactionSpotPrices: mockTransactionSpotPrices,
        defaultCurrencies: { fiat: 'USD', crypto: 'ETH' },
        fullTokenList: mockNewAssetOptions
      })}>
        <LibContext.Provider value={{
          ...MockedLib as any,
          getERC20Allowance
        }}>
          <ConfirmTransactionPanel
            onConfirm={onConfirmTransaction}
            onReject={onRejectTransaction}
          />
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
  const onSign = () => {
    alert('Signed Data')
  }

  const onCancel = () => {
    alert('Canceled Signing Data')
  }

  const signMessageDataPayload = [{
    id: 0,
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    message: 'To avoid digital cat burglars, sign below to authenticate with CryptoKitties.',
    domain: '',
    originInfo: mockOriginInfo,
    coin: BraveWallet.CoinType.ETH,
    isEip712: true,
    domainHash: '',
    primaryHash: '',
    messageBytes: undefined
  }]

  return (
    <StyledExtensionWrapperLonger>
      <SignPanel
        signMessageData={signMessageDataPayload}
        accounts={mockAccounts}
        selectedNetwork={mockNetworks[0]}
        onCancel={onCancel}
        onSign={onSign}
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
      <EncryptionKeyPanel
        panelType='request'
        decryptPayload={mockDecryptRequest}
        encryptionKeyPayload={mockEncryptionKeyRequest}
        accounts={mockAccounts}
        selectedNetwork={mockNetworks[0]}
        onCancel={onCancel}
        onProvideOrAllow={onProvide}
        eTldPlusOne={originInfo.eTldPlusOne}
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
      <EncryptionKeyPanel
        panelType='read'
        encryptionKeyPayload={mockEncryptionKeyRequest}
        decryptPayload={mockDecryptRequest}
        accounts={mockAccounts}
        selectedNetwork={mockNetworks[0]}
        onCancel={onCancel}
        onProvideOrAllow={onAllow}
        eTldPlusOne={originInfo.eTldPlusOne}
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
  const [selectedAccount, setSelectedAccount] = React.useState<WalletAccountType>(
    mockAccounts[0]
  )
  const [favoriteApps, setFavoriteApps] = React.useState<BraveWallet.AppItem[]>([
    AppsList()[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<
    AppsListType[]
  >(AppsList())
  const [selectedTransaction, setSelectedTransaction] = React.useState<
    SerializableTransactionInfo | undefined
  >(transactionList[1][0])

  const onBack = () => {
    setSelectedPanel('main')
  }

  const onBackToTransactions = () => {
    navigateTo('activity')
  }

  const onSelectAccount = (account: WalletAccountType) => () => {
    setSelectedAccount(account)
    setSelectedPanel('main')
  }

  const getTitle = (path: PanelTypes) => {
    const title = PanelTitles().find((title) => path === title.id)
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
    const newList = favoriteApps.filter(
      (fav) => fav.name !== app.name
    )
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

  const onAddAsset = () => {
    alert('Will redirect to brave://wallet/crypto/portfolio/add-asset')
  }

  const onClickRetryTransaction = () => {
    // Does nothing in storybook
    alert('Will retry transaction')
  }

  const onClickCancelTransaction = () => {
    // Does nothing in storybook
    alert('Will cancel transaction')
  }

  const onClickSpeedupTransaction = () => {
    // Does nothing in storybook
    alert('Will speedup transaction')
  }

  const onSelectTransaction = (transaction: SerializableTransactionInfo) => {
    navigateTo('transactionDetails')
    setSelectedTransaction(transaction)
    console.log(selectedTransaction)
  }

  return (
    <WalletPanelStory>
      <StyledExtensionWrapper>
        {walletLocked ? (
          <LockPanel onSubmit={unlockWallet} onClickRestore={onRestore} />
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
                    />
                  </SelectContainer>
                )}
                {selectedPanel === 'transactionDetails' &&
                  selectedTransaction && (
                    <SelectContainer>
                      <TransactionDetailPanel
                        transaction={selectedTransaction}
                        onBack={onBackToTransactions}
                        onCancelTransaction={onClickCancelTransaction}
                        onRetryTransaction={onClickRetryTransaction}
                        onSpeedupTransaction={onClickSpeedupTransaction}
                        accounts={mockAccounts}
                        defaultCurrencies={mockDefaultCurrencies}
                        selectedNetwork={mockNetworks[0]}
                        visibleTokens={mockNewAssetOptions}
                        transactionSpotPrices={[]}
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
                            onSelectTransaction={onSelectTransaction}
                            selectedNetwork={mockNetworks[0]}
                            selectedAccountAddress={
                              mockedTransactionAccounts[0].address
                            }
                          />
                        )}
                        {selectedPanel === 'assets' && (
                          <AssetsPanel
                            selectedAccount={selectedAccount}
                            userAssetList={mockAccountAssetOptions}
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
  const onCancel = (accountAddress: string, coinType: BraveWallet.CoinType) => {
    // Doesn't do anything in storybook
  }

  const onConfirmTransaction = () => {
    // Doesn't do anything in storybook
  }

  const onClickInstructions = () => {
    // Open support link in new tab
    window.open('https://support.brave.com/hc/en-us/articles/4409309138701', '_blank', 'noreferrer')
  }

  return (
    <StyledExtensionWrapper>
      <ConnectHardwareWalletPanel
        walletName='Ledger 1'
        accountAddress='0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14'
        coinType={BraveWallet.CoinType.ETH}
        onCancel={onCancel}
        retryCallable={onConfirmTransaction}
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
  const onCancel = () => {
    // Doesn't do anything in storybook
  }

  const onAddToken = () => {
    // Doesn't do anything in storybook
  }

  return (
    <StyledExtensionWrapper>
      <AddSuggestedTokenPanel
        onCancel={onCancel}
        onAddToken={onAddToken}
        originInfo={originInfo}
        token={mockNewAssetOptions[2]}
        selectedNetwork={mockNetworks[0]}
      />
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
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
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
              onCancelTransaction={mockedFunction}
              onRetryTransaction={mockedFunction}
              onSpeedupTransaction={mockedFunction}
              accounts={mockedTransactionAccounts}
              defaultCurrencies={mockDefaultCurrencies}
              selectedNetwork={mockNetworks[0]}
              visibleTokens={mockNewAssetOptions}
              transactionSpotPrices={[]}
              transaction={tx}
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
  const onSelectTransaction = () => {
    // Doesn't do anything in storybook
  }

  const navigateTo = () => {
    // Doesn't do anything in storybook
  }

  return (
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledExtensionWrapper>
        <Panel
          navAction={navigateTo}
          title={'Recent Transactions'}
          useSearch={false}
          searchAction={undefined}
        >
          <ScrollContainer>
            <TransactionsPanel
              onSelectTransaction={onSelectTransaction}
              selectedNetwork={mockNetworks[0]}
              selectedAccountAddress={mockedTransactionAccounts[0].address}
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
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledCreateAccountPanel>
        <CreateAccountTab
          prevNetwork={mockNetworks[0]}
        />
      </StyledCreateAccountPanel>
    </Provider>
  )
}

_CreateAccount.story = {
  name: 'Create Account Tab'
}
