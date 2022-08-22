// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { combineReducers, createStore } from 'redux'

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
import {
  Send,
  Buy,
  SelectAsset,
  SelectNetworkWithHeader,
  SelectAccount,
  CreateAccountTab
} from '../components/buy-send-swap'
import { TransactionSubmitted } from '../components/extension/post-confirmation/submitted'
import { TransactionConfirming } from '../components/extension/post-confirmation/confirming'
import { TransactionFailed } from '../components/extension/post-confirmation/failed'
import { TransactionComplete } from '../components/extension/post-confirmation/complete'
import {
  BraveWallet,
  WalletAccountType,
  PanelTypes,
  AppsListType,
  BuySendSwapViewTypes,
  WalletState,
  AccountTransactions
} from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import { filterAppList } from '../utils/filter-app-list'
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
import './locale'
import { LibContext } from '../common/context/lib.context'
import { createSendCryptoReducer } from '../common/reducers/send_crypto_reducer'
import { createWalletReducer } from '../common/reducers/wallet_reducer'
import { createPageReducer } from '../page/reducers/page_reducer'

// mocks
import * as MockedLib from '../common/async/__mocks__/lib'
import { mockedErc20ApprovalTransaction, mockTransactionInfo } from './mock-data/mock-transaction-info'
import { mockDefaultCurrencies } from './mock-data/mock-default-currencies'
import { mockTransactionSpotPrices } from './mock-data/current-price-data'
import { mockAccounts, mockedTransactionAccounts } from './mock-data/mock-wallet-accounts'
import { mockEncryptionKeyRequest, mockDecryptRequest } from './mock-data/mock-encryption-key-payload'
import { mockOriginInfo } from './mock-data/mock-origin-info'
import { mockAccountAssetOptions, mockBasicAttentionToken, mockEthToken, mockNewAssetOptions } from './mock-data/mock-asset-options'
import { mockPageState } from './mock-data/mock-page-state'
import { mockWalletState } from './mock-data/mock-wallet-state'
import { mockSendCryptoState } from './mock-data/send-crypto-state'
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
            value: '0xb1a2bc2ec50000'
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
      createdTime: { microseconds: BigInt((Date.now() * 1000) - 1000 * 60 * 5 * 1000) },
      submittedTime: { microseconds: BigInt((Date.now() * 1000) - 1000 * 60 * 5) },
      confirmedTime: { microseconds: BigInt((Date.now() * 1000) - 1000 * 60 * 5) },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
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
            value: '0xb1a2bc2ec50000'
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
      createdTime: { microseconds: BigInt(0) },
      submittedTime: { microseconds: BigInt(0) },
      confirmedTime: { microseconds: BigInt(0) },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
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
            value: '0xb1a2bc2ec90000'
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
      createdTime: { microseconds: BigInt(0) },
      submittedTime: { microseconds: BigInt(0) },
      confirmedTime: { microseconds: BigInt(0) },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
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
            value: '0xb1a2bc2ec90000'
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
      createdTime: { microseconds: BigInt(0) },
      submittedTime: { microseconds: BigInt(0) },
      confirmedTime: { microseconds: BigInt(0) },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
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
            value: '0xb1a2bc2ec90000'
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
      createdTime: { microseconds: BigInt(0) },
      submittedTime: { microseconds: BigInt(0) },
      confirmedTime: { microseconds: BigInt(0) },
      originInfo: mockOriginInfo,
      groupId: undefined
    }
  ],
  [mockUserAccounts[1].id]: [
    {
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
            value: '0xb1a2bc2ec90000'
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
      createdTime: { microseconds: BigInt(0) },
      submittedTime: { microseconds: BigInt(0) },
      confirmedTime: { microseconds: BigInt(0) },
      originInfo: mockOriginInfo,
      groupId: undefined
    },
    {
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
            value: '0xb1a2bc2ec90000'
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
      createdTime: { microseconds: BigInt(0) },
      submittedTime: { microseconds: BigInt(0) },
      confirmedTime: { microseconds: BigInt(0) },
      originInfo: mockOriginInfo,
      groupId: undefined
    }
  ]
}

const originInfo = mockOriginInfo

const store = createStore(combineReducers({
  wallet: createWalletReducer(mockWalletState),
  page: createPageReducer(mockPageState),
  sendCrypto: createSendCryptoReducer(mockSendCryptoState)
}))

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

function createStoreWithCustomState (customWalletState: Partial<WalletState> = {}) {
  return createStore(combineReducers({
    wallet: createWalletReducer({
      ...mockWalletState,
      ...customWalletState
    }),
    page: createPageReducer(mockPageState)
  }))
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
        defaultNetworks={mockNetworks}
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
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([
    mockAccounts[0]
  ])
  const [readyToConnect, setReadyToConnect] = React.useState<boolean>(false)
  const selectAccount = (account: WalletAccountType) => {
    const newList = [...selectedAccounts, account]
    setSelectedAccounts(newList)
  }
  const removeAccount = (account: WalletAccountType) => {
    const newList = selectedAccounts.filter(
      (accounts) => accounts.id !== account.id
    )
    setSelectedAccounts(newList)
  }
  const onSubmit = () => {
    alert(`Connecting to ${originInfo.originSpec} using: ${JSON.stringify(selectedAccounts)}`)
  }
  const primaryAction = () => {
    if (!readyToConnect) {
      setReadyToConnect(true)
    } else {
      onSubmit()
    }
  }
  const secondaryAction = () => {
    if (readyToConnect) {
      setReadyToConnect(false)
    } else {
      alert('You Clicked The Cancel Button!')
    }
  }
  return (
    <StyledExtensionWrapperLonger>
      <ConnectWithSite
        originInfo={originInfo}
        isReady={readyToConnect}
        accounts={mockAccounts}
        primaryAction={primaryAction}
        secondaryAction={secondaryAction}
        selectAccount={selectAccount}
        removeAccount={removeAccount}
        selectedAccounts={selectedAccounts}
      />
    </StyledExtensionWrapperLonger>
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
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList())
  const [selectedNetwork] = React.useState<BraveWallet.NetworkInfo>(mockNetworks[0])
  const [selectedWyreAsset, setSelectedWyreAsset] = React.useState<BraveWallet.BlockchainToken>(mockEthToken)
  const [, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken>(mockBasicAttentionToken)
  const [showSelectAsset, setShowSelectAsset] = React.useState<boolean>(false)
  const [selectedTransaction, setSelectedTransaction] = React.useState<BraveWallet.TransactionInfo | undefined>(transactionList[1][0])

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    if (view === 'assets') {
      setShowSelectAsset(true)
    }
  }

  const onBack = () => {
    setSelectedPanel('main')
  }

  const onBackToTransactions = () => {
    navigateTo('transactions')
  }

  const onSelectAccount = (account: WalletAccountType) => () => {
    setSelectedAccount(account)
    setSelectedPanel('main')
  }

  const onHideSelectAsset = () => {
    setShowSelectAsset(false)
  }

  const onSelectAsset = (asset: BraveWallet.BlockchainToken) => () => {
    if (selectedPanel === 'buy') {
      setSelectedWyreAsset(asset)
    } else {
      setSelectedAsset(asset)
    }
    setShowSelectAsset(false)
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

  const onSelectTransaction = (transaction: BraveWallet.TransactionInfo) => {
    navigateTo('transactionDetails')
    setSelectedTransaction(transaction)
    console.log(selectedTransaction)
  }

  const onShowCurrencySelection = React.useCallback(() => {
    navigateTo('currencies')
  }, [])

  return (
    <Provider store={store}>
      <StyledExtensionWrapper>
        {walletLocked ? (
          <LockPanel
            onSubmit={unlockWallet}
            onClickRestore={onRestore}
          />
        ) : (
          <>
            {selectedPanel === 'main' ? (
              <ConnectedPanel
                navAction={navigateTo}
                isSwapSupported={true}
              />
            ) : (
              <>
                {showSelectAsset &&
                  <SelectContainer>
                    <SelectAsset
                      assets={mockAccountAssetOptions}
                      onSelectAsset={onSelectAsset}
                      onBack={onHideSelectAsset}
                    />
                  </SelectContainer>
                }
                {selectedPanel === 'accounts' &&
                  <SelectContainer>
                    <SelectAccount
                      accounts={mockAccounts}
                      onBack={onBack}
                      onSelectAccount={onSelectAccount}
                      onAddAccount={onAddAccount}
                      hasAddButton={true}
                      selectedAccount={selectedAccount}
                    />
                  </SelectContainer>
                }
                {selectedPanel === 'networks' &&
                  <SelectContainer>
                    <SelectNetworkWithHeader
                      onBack={onBack}
                      hasAddButton={true}
                      onAddNetwork={onAddNetwork}
                    />
                  </SelectContainer>
                }
                {selectedPanel === 'transactionDetails' && selectedTransaction &&
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
                }
                {!showSelectAsset && selectedPanel !== 'networks' && selectedPanel !== 'accounts' && selectedPanel !== 'transactionDetails' &&
                  < Panel
                    navAction={navigateTo}
                    title={panelTitle}
                    useSearch={selectedPanel === 'apps'}
                    searchAction={selectedPanel === 'apps' ? filterList : undefined}
                  >
                    <ScrollContainer>
                      {selectedPanel === 'apps' &&
                        <AppList
                          list={filteredAppsList}
                          favApps={favoriteApps}
                          addToFav={addToFavorites}
                          removeFromFav={removeFromFavorites}
                          action={browseMore}
                        />
                      }
                      {selectedPanel === 'send' &&
                        <Send
                          onChangeSendView={onChangeSendView}
                        />
                      }
                      {selectedPanel === 'buy' &&
                        <Buy
                          onChangeBuyView={onChangeSendView}
                          selectedAsset={selectedWyreAsset}
                          onShowCurrencySelection={onShowCurrencySelection}
                        />
                      }
                      {selectedPanel === 'sitePermissions' &&
                        <SitePermissions />
                      }
                      {selectedPanel === 'transactions' &&
                        <TransactionsPanel
                          accounts={mockedTransactionAccounts}
                          defaultCurrencies={mockDefaultCurrencies}
                          onSelectTransaction={onSelectTransaction}
                          selectedNetwork={mockNetworks[0]}
                          selectedAccount={mockedTransactionAccounts[0]}
                          visibleTokens={mockNewAssetOptions}
                          transactionSpotPrices={[]}
                          transactions={transactionList}

                        />
                      }
                      {selectedPanel === 'assets' &&
                        <AssetsPanel
                          defaultCurrencies={mockDefaultCurrencies}
                          selectedAccount={selectedAccount}
                          spotPrices={[]}
                          userAssetList={mockAccountAssetOptions}
                          onAddAsset={onAddAsset}
                          networkList={[selectedNetwork]}
                        />
                      }
                    </ScrollContainer>
                  </Panel>
                }
              </>
            )}
          </>
        )
        }
      </StyledExtensionWrapper>
    </Provider>
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
    window.open('https://support.brave.com/hc/en-us/articles/4409309138701', '_blank')
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
              accounts={mockedTransactionAccounts}
              defaultCurrencies={mockDefaultCurrencies}
              onSelectTransaction={onSelectTransaction}
              selectedNetwork={mockNetworks[0]}
              selectedAccount={mockedTransactionAccounts[0]}
              visibleTokens={mockNewAssetOptions}
              transactionSpotPrices={[{ assetTimeframeChange: '', fromAsset: 'ETH', toAsset: 'USD', price: '2500' }]}
              transactions={transactionList}
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

export const _TransactionSubmitted = () => {
  return (
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledExtensionWrapper>
        <TransactionSubmitted
          headerTitle='Swap 0.01 ETH to 32.2583 USDC'
          onClose={() => alert('Close panel screen')}
        />
      </StyledExtensionWrapper>
    </Provider>
  )
}

_TransactionSubmitted.story = {
  name: 'Transaction Submitted'
}

export const _TransactionConfirming = () => {
  return (
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledExtensionWrapper>
        <TransactionConfirming
          headerTitle='Swap 0.01 ETH to 32.2583 USDC'
          confirmations={8}
          confirmationsNeeded={12}
          onClose={() => alert('Close panel screen')}
        />
      </StyledExtensionWrapper>
    </Provider>
  )
}

_TransactionConfirming.story = {
  name: 'Transaction Confirming'
}

export const _TransactionComplete = () => {
  return (
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledExtensionWrapper>
        <TransactionComplete
          headerTitle='Swap 0.01 ETH to 32.2583 USDC'
          description='32.2583 USDC has been successfully deposited into your wallet.'
          isPrimaryCTADisabled={false}
          onClose={() => alert('Close panel screen')}
          onClickPrimaryCTA={() => alert('Clicked primary CTA')}
          onClickSecondaryCTA={() => alert('Clicked secondary CTA')}
        />
      </StyledExtensionWrapper>
    </Provider>
  )
}

_TransactionComplete.story = {
  name: 'Transaction Complete'
}

export const _TransactionFailed = () => {
  return (
    <Provider store={createStoreWithCustomState(mockCustomStoreState)}>
      <StyledExtensionWrapper>
        <TransactionFailed
          headerTitle='Swap 0.01 ETH to 32.2583 USDC'
          isPrimaryCTADisabled={false}
          errorDetailTitle='Try raising slippage'
          errorDetailContent='[ethjs-query] while formatting outputs from RPC ‘{“value”: {“code”:-32603,”data”: {“code”-32603,”message”:”Internal error”}}}’'
          onClose={() => alert('Close panel screen')}
          onClickPrimaryCTA={() => alert('Clicked primary CTA')}
          onClickSecondaryCTA={() => alert('Clicked secondary CTA')}
        />
      </StyledExtensionWrapper>
    </Provider>
  )
}

_TransactionFailed.story = {
  name: 'Transaction Failed'
}
