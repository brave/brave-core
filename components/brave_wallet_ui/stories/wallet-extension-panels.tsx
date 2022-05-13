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
import {
  BraveWallet,
  WalletAccountType,
  PanelTypes,
  AppsListType,
  BuySendSwapViewTypes,
  Origin,
  WalletState
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
import { transactionDummyData } from './wallet-concept'
import { mockedErc20ApprovalTransaction, mockTransactionInfo } from './mock-data/mock-transaction-info'
import { mockDefaultCurrencies } from './mock-data/mock-default-currencies'
import { mockTransactionSpotPrices } from './mock-data/current-price-data'
import { mockAccounts, mockedTransactionAccounts } from './mock-data/mock-wallet-accounts'
import { mockEncryptionKeyRequest, mockDecryptRequest } from './mock-data/mock-encryption-key-payload'
import { mockOriginInfo } from './mock-data/mock-origin-info'
import { mockAccountAssetOptions, mockBasicAttentionToken, mockEthToken, mockNewAssetOptions } from './mock-data/mock-asset-options'
import { LibContext } from '../common/context/lib.context'
import * as MockedLib from '../common/async/__mocks__/lib'

import { createSendCryptoReducer } from '../common/reducers/send_crypto_reducer'
import { createWalletReducer } from '../common/reducers/wallet_reducer'
import { createPageReducer } from '../page/reducers/page_reducer'
import { mockPageState } from './mock-data/mock-page-state'
import { mockWalletState } from './mock-data/mock-wallet-state'
import { mockSendCryptoState } from './mock-data/send-crypto-state'
export default {
  title: 'Wallet/Extension/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
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

  const onLearnMore = () => {
    alert('Will nav to Learn More')
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
        onLearnMore={onLearnMore}
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
    originInfo: mockOriginInfo
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
  const [inputValue, setInputValue] = React.useState<string>('')
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
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)
  const [selectedNetwork] = React.useState<BraveWallet.NetworkInfo>(mockNetworks[0])
  const [selectedWyreAsset, setSelectedWyreAsset] = React.useState<BraveWallet.BlockchainToken>(mockEthToken)
  const [, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken>(mockBasicAttentionToken)
  const [showSelectAsset, setShowSelectAsset] = React.useState<boolean>(false)
  const [buyAmount, setBuyAmount] = React.useState('')
  const [selectedTransaction, setSelectedTransaction] = React.useState<BraveWallet.TransactionInfo | undefined>(transactionList[1][0])
  const [selectedBuyOption, setSelectedBuyOption] = React.useState(BraveWallet.OnRampProvider.kRamp)

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

  const onSubmitBuy = () => {
    alert(`Buy ${selectedWyreAsset.symbol} asset`)
  }

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

  const unlockWallet = () => {
    if (inputValue !== 'password') {
      setHasPasswordError(true)
    } else {
      setWalletLocked(false)
    }
  }

  const onLockWallet = () => {
    setWalletLocked(true)
  }

  const handlePasswordChanged = (value: string) => {
    setHasPasswordError(false)
    setInputValue(value)
  }

  const onOpenSettings = () => {
    alert('Will go to Wallet Settings')
  }

  const onRestore = () => {
    alert('Will navigate to full wallet restore page')
  }

  const onDisconnectFromOrigin = (origin: Origin, account: WalletAccountType, connectedAccounts: WalletAccountType[]) => {
    console.log(`Will disconnect ${account.address} from ${origin.host}`)
  }

  const onConnectToOrigin = (origin: Origin, account: WalletAccountType) => {
    console.log(`Will connect ${account.address} to ${origin.host}`)
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

  const connectedAccounts = mockAccounts.slice(0, 2)

  const onSelectTransaction = (transaction: BraveWallet.TransactionInfo) => {
    navigateTo('transactionDetails')
    setSelectedTransaction(transaction)
    console.log(selectedTransaction)
  }

  return (
    <Provider store={store}>
      <StyledExtensionWrapper>
        {walletLocked ? (
          <LockPanel
            hasPasswordError={hasPasswordError}
            onSubmit={unlockWallet}
            disabled={inputValue === ''}
            onPasswordChanged={handlePasswordChanged}
            onClickRestore={onRestore}
          />
        ) : (
          <>
            {selectedPanel === 'main' ? (
              <ConnectedPanel
                spotPrices={[]}
                defaultCurrencies={mockDefaultCurrencies}
                selectedNetwork={selectedNetwork}
                selectedAccount={selectedAccount}
                isConnected={true}
                navAction={navigateTo}
                onLockWallet={onLockWallet}
                onOpenSettings={onOpenSettings}
                originInfo={originInfo}
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
                          defaultCurrencies={mockDefaultCurrencies}
                          onChangeBuyView={onChangeSendView}
                          onInputChange={onSetBuyAmount}
                          onSubmit={onSubmitBuy}
                          selectedAsset={selectedWyreAsset}
                          buyAmount={buyAmount}
                          selectedNetwork={selectedNetwork}
                          networkList={[]}
                          selectedBuyOption={selectedBuyOption}
                          onSelectBuyOption={setSelectedBuyOption}
                          rampAssetOptions={mockAccountAssetOptions}
                          wyreAssetOptions={mockAccountAssetOptions} />
                      }
                      {selectedPanel === 'sitePermissions' &&
                        <SitePermissions
                          selectedAccount={selectedAccount}
                          originInfo={originInfo}
                          onDisconnect={onDisconnectFromOrigin}
                          connectedAccounts={connectedAccounts}
                          accounts={mockAccounts}
                          onSwitchAccount={onSelectAccount}
                          onConnect={onConnectToOrigin}
                          onAddAccount={onAddAccount}
                        />
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
  const onCancel = () => {
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
    <StyledCreateAccountPanel>
      <CreateAccountTab
        prevNetwork={mockNetworks[0]}
      />
    </StyledCreateAccountPanel>
  )
}

_CreateAccount.story = {
  name: 'Create Account Tab'
}
