import * as React from 'react'
import { combineReducers, createStore } from 'redux'
import { WalletWidgetStandIn } from './style'
import {
  WalletPageLayout,
  WalletSubViewLayout,
  LockScreen,
  OnboardingRestore
} from '../components/desktop'
import {
  NavTypes,
  BraveWallet,
  RPCResponseType,
  UserAccountType,
  AccountTransactions,
  BuySendSwapTypes,
  WalletAccountType
} from '../constants/types'
import Onboarding from './screens/onboarding'
import BackupWallet from './screens/backup-wallet'
import CryptoStoryView from './screens/crypto-story-view'
import './locale'
import BuySendSwap from './screens/buy-send-swap'
import { mockRecoveryPhrase, mockUserAccounts } from './mock-data/user-accounts'
import { mockRPCResponse } from './mock-data/rpc-response'
import { mockUserWalletPreferences } from './mock-data/user-wallet-preferences'
import { getLocale } from '../../common/locale'
import {
  HardwareWalletConnectOpts
} from '../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { mockNetworks } from './mock-data/mock-networks'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { Provider } from 'react-redux'
import { createWalletReducer } from '../common/reducers/wallet_reducer'
import { mockWalletState } from './mock-data/mock-wallet-state'
import { mockPageState } from './mock-data/mock-page-state'
import { createPageReducer } from '../page/reducers/page_reducer'
import * as Lib from '../common/async/__mocks__/lib'
import { LibContext } from '../common/context/lib.context'
import { mockAccountAssetOptions, mockNewAssetOptions } from './mock-data/mock-asset-options'
import { createSendCryptoReducer } from '../common/reducers/send_crypto_reducer'
import { mockSendCryptoState } from './mock-data/send-crypto-state'
import { mockOriginInfo } from './mock-data/mock-origin-info'

const store = createStore(combineReducers({
  wallet: createWalletReducer(mockWalletState),
  page: createPageReducer(mockPageState),
  sendCrypto: createSendCryptoReducer(mockSendCryptoState)
}))

export default {
  title: 'Wallet/Desktop',
  argTypes: {
    onboarding: { control: { type: 'boolean', onboard: false } },
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const transactionDummyData: AccountTransactions = {
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
      originInfo: mockOriginInfo
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
      originInfo: mockOriginInfo
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
      originInfo: mockOriginInfo
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
      originInfo: mockOriginInfo
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
      originInfo: mockOriginInfo
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
      originInfo: mockOriginInfo
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
      originInfo: mockOriginInfo
    }
  ]
}

export const _DesktopWalletConcept = (args: { onboarding: boolean, locked: boolean }) => {
  const {
    onboarding,
    locked
  } = args
  const [view] = React.useState<NavTypes>('crypto')
  const [isFilecoinEnabled] = React.useState<boolean>(true)
  const [isSolanaEnabled] = React.useState<boolean>(true)
  const [needsOnboarding] = React.useState<boolean>(onboarding)
  const [walletLocked, setWalletLocked] = React.useState<boolean>(locked)
  const [needsBackup, setNeedsBackup] = React.useState<boolean>(true)
  const [showBackup, setShowBackup] = React.useState<boolean>(false)
  const [inputValue, setInputValue] = React.useState<string>('')
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)
  const [selectedAsset] = React.useState<BraveWallet.BlockchainToken>()
  const [selectedNetwork] = React.useState<BraveWallet.NetworkInfo>(mockNetworks[0])
  const [, setSelectedAccount] = React.useState<UserAccountType>(mockUserAccounts[0])
  const [showAddModal, setShowAddModal] = React.useState<boolean>(false)
  const [buyAmount, setBuyAmount] = React.useState('')
  const [isRestoring, setIsRestoring] = React.useState<boolean>(false)
  const [importAccountError, setImportAccountError] = React.useState<boolean>(false)
  const [selectedWidgetTab, setSelectedWidgetTab] = React.useState<BuySendSwapTypes>('buy')
  const [showVisibleAssetsModal, setShowVisibleAssetsModal] = React.useState<boolean>(false)

  const onToggleRestore = () => {
    setIsRestoring(!isRestoring)
  }

  // In the future these will be actual paths
  // for example wallet/rewards
  // const navigateTo = (path: NavTypes) => {
  //   setView(path)
  // }

  const onWalletBackedUp = () => {
    setNeedsBackup(false)
  }

  const unlockWallet = () => {
    if (inputValue !== 'password') {
      setHasPasswordError(true)
    } else {
      setWalletLocked(false)
    }
  }

  const lockWallet = () => {
    setWalletLocked(true)
  }

  const handlePasswordChanged = (value: string) => {
    setHasPasswordError(false)
    setInputValue(value)
  }

  const onUpdateAccountName = (): { success: boolean } => {
    return { success: true }
  }

  const onShowBackup = () => {
    setShowBackup(true)
  }

  const onHideBackup = () => {
    setShowBackup(false)
  }

  // This returns info about a single asset
  const assetInfo = (account: RPCResponseType) => {
    return account.assets.find((a) => a.symbol === selectedAsset?.symbol)
  }

  // This returns the balance of a single accounts asset
  const singleAccountBalance = (account: RPCResponseType) => {
    const balance = assetInfo(account)?.balance
    return balance ? balance.toString() : ''
  }

  const accountInfo = (asset: RPCResponseType) => {
    const foundAccount = mockUserAccounts.find((account) => account.address === asset.address)
    return foundAccount
  }

  // This returns a list of accounts with a balance of the selected asset
  const accounts = React.useMemo(() => {
    const id = selectedAsset ? selectedAsset.symbol : ''
    const list = selectedAsset ? mockRPCResponse.filter((account) => account.assets.map((assetID) => assetID.symbol).includes(id)) : mockRPCResponse
    const newList = list.map((wallet) => {
      const walletInfo = accountInfo(wallet)
      const id = walletInfo ? walletInfo.id : ''
      const name = walletInfo ? walletInfo.name : getLocale('braveWalletAccount')
      return {
        id: id,
        name: name,
        address: wallet.address,
        nativeBalanceRegistry: {
          '0x1': singleAccountBalance(wallet)
        },
        asset: selectedAsset ? selectedAsset.symbol : '',
        accountType: 'Primary',
        tokenBalanceRegistry: {},
        coin: BraveWallet.CoinType.ETH
      } as WalletAccountType
    })
    return newList
  }, [selectedAsset, mockRPCResponse])

  // This will scrape all of the user's accounts and combine the balances for a single asset
  const scrapedFullAssetBalance = (asset: BraveWallet.BlockchainToken) => {
    const response = mockRPCResponse
    const amounts = response.map((account) => {
      const balance = account.assets.find((item) => item.id === asset.contractAddress)?.balance
      return balance || 0
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  const userAssetList = React.useMemo(() => {
    const userList = mockUserWalletPreferences.viewableAssets
    const newList = mockNewAssetOptions.filter((asset) => userList.includes(asset.contractAddress))
    return newList.map((asset) => {
      return {
        asset: asset,
        assetBalance: scrapedFullAssetBalance(asset).toString()
      }
    })
  }, [mockUserWalletPreferences.viewableAssets])

  const onCreateAccount = (name: string) => {
    alert(name)
  }

  const onImportAccount = (name: string, key: string, coin: BraveWallet.CoinType) => {
    // doesnt do anything in storybook
  }
  const onImportFilecoinAccount = (accountName: string, privateKey: string, network: string) => {
    // doesnt do anything in storybook
  }

  const onImportAccountFromJson = (name: string, password: string, json: string) => {
    // doesnt do anything in storybook
  }

  const onToggleAddModal = () => {
    setShowAddModal(!showAddModal)
  }

  const onSubmitBuy = (asset: BraveWallet.BlockchainToken) => {
    alert(`Buy ${asset.symbol} asset`)
  }

  const onSelectAccount = (account: UserAccountType) => {
    setSelectedAccount(account)
  }

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

  const onRemoveAccount = () => {
    alert('Will Remove Account')
  }

  const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Promise<BraveWallet.HardwareWalletAccount[]> => {
    const makeDerivationPath = (index: number): string => `m/44'/60'/${index}'/0/0`

    return new Promise((resolve) => {
      resolve(Array.from({ length: opts.stopIndex - opts.startIndex }, (_, i) => ({
        address: '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2',
        derivationPath: makeDerivationPath(i + opts.startIndex),
        name: 'Ledger 1',
        hardwareVendor: 'Ledger',
        deviceId: 'device1',
        coin: BraveWallet.CoinType.ETH
      })))
    })
  }

  const getBalance = (address: string): Promise<string> => {
    return new Promise(async (resolve) => {
      resolve('0')
    })
  }

  const onAddHardwareAccounts = (accounts: BraveWallet.HardwareWalletAccount[]) => {
    console.log(accounts)
  }

  const onViewPrivateKey = () => {
    // Doesnt do anything in storybook
  }

  const onDoneViewingPrivateKey = () => {
    // Doesnt do anything in storybook
  }

  const onSetImportAccountError = (hasError: boolean) => setImportAccountError(hasError)

  const defaultCurrencies = {
    fiat: 'USD',
    crypto: 'BTC'
  }

  const onShowVisibleAssetsModal = (value: boolean) => {
    setShowVisibleAssetsModal(value)
  }

  return (
    <Provider store={store}>
      <LibContext.Provider value={Lib as any}>
        <WalletPageLayout>
          {/* <SideNav
            navList={NavOptions}
            selectedButton={view}
            onSubmit={navigateTo}
          /> */}
          <WalletSubViewLayout>
            {isRestoring ? (
              <OnboardingRestore />
            ) : (
              <>
                {needsOnboarding
                  ? (
                    <Onboarding />
                  ) : (
                    <>
                      {view === 'crypto' ? (
                        <>
                          {walletLocked ? (
                            <LockScreen
                              hasPasswordError={hasPasswordError}
                              onSubmit={unlockWallet}
                              disabled={inputValue === ''}
                              onPasswordChanged={handlePasswordChanged}
                              onShowRestore={onToggleRestore}
                            />
                          ) : (
                            <>
                              {showBackup ? (
                                <BackupWallet
                                  isOnboarding={false}
                                  onCancel={onHideBackup}
                                  onSubmit={onWalletBackedUp}
                                  recoveryPhrase={mockRecoveryPhrase}
                                />
                              ) : (
                                <CryptoStoryView
                                  defaultCurrencies={defaultCurrencies}
                                  onLockWallet={lockWallet}
                                  needsBackup={needsBackup}
                                  onShowBackup={onShowBackup}
                                  accounts={accounts}
                                  transactions={transactionDummyData}
                                  userAssetList={userAssetList}
                                  onCreateAccount={onCreateAccount}
                                  onImportAccount={onImportAccount}
                                  isFilecoinEnabled={isFilecoinEnabled}
                                  isSolanaEnabled={isSolanaEnabled}
                                  onImportFilecoinAccount={onImportFilecoinAccount}
                                  onConnectHardwareWallet={onConnectHardwareWallet}
                                  onAddHardwareAccounts={onAddHardwareAccounts}
                                  getBalance={getBalance}
                                  showAddModal={showAddModal}
                                  onToggleAddModal={onToggleAddModal}
                                  onUpdateAccountName={onUpdateAccountName}
                                  selectedNetwork={selectedNetwork}
                                  onRemoveAccount={onRemoveAccount}
                                  privateKey='gf65a4g6a54fg6a54fg6ad4fa5df65a4d6ff54a6sdf'
                                  onDoneViewingPrivateKey={onDoneViewingPrivateKey}
                                  onViewPrivateKey={onViewPrivateKey}
                                  networkList={mockNetworks}
                                  onImportAccountFromJson={onImportAccountFromJson}
                                  hasImportError={importAccountError}
                                  onSetImportError={onSetImportAccountError}
                                  userVisibleTokensInfo={[]}
                                  onShowVisibleAssetsModal={onShowVisibleAssetsModal}
                                  showVisibleAssetsModal={showVisibleAssetsModal}
                                />
                              )}
                            </>
                          )}
                        </>
                      ) : (
                        <div style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
                          <h2>{view} view</h2>
                        </div>
                      )}
                    </>
                  )}
              </>
            )}

          </WalletSubViewLayout>
          {!needsOnboarding && !walletLocked &&
            <WalletWidgetStandIn>
              <BuySendSwap
                selectedTab={selectedWidgetTab}
                buyAmount={buyAmount}
                onSubmitBuy={onSubmitBuy}
                onSetBuyAmount={onSetBuyAmount}
                onSelectAccount={onSelectAccount}
                onSelectTab={setSelectedWidgetTab}
                buyAssetOptions={mockAccountAssetOptions}
              />
              <SweepstakesBanner />
            </WalletWidgetStandIn>
          }
        </WalletPageLayout>
      </LibContext.Provider>
    </Provider>
  )
}

_DesktopWalletConcept.args = {
  onboarding: false,
  locked: false
}

_DesktopWalletConcept.story = {
  name: 'Concept'
}
