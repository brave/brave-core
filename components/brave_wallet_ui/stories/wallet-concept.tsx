import * as React from 'react'
import { WalletWidgetStandIn } from './style'
import {
  // SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView,
  LockScreen
} from '../components/desktop'
import {
  NavTypes,
  AssetPriceTimeframe,
  PriceDataObjectType,
  AssetOptionType,
  AssetPriceInfo,
  RPCResponseType,
  OrderTypes,
  UserAccountType,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  Network
} from '../constants/types'
import Onboarding from './screens/onboarding'
import BackupWallet from './screens/backup-wallet'
import * as Result from '../common/types/result'

// import { NavOptions } from '../options/side-nav-options'
import { AssetOptions } from '../options/asset-options'
import { SlippagePresetOptions } from '../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../options/expiration-preset-options'
import BuySendSwap from './screens/buy-send-swap'
import { recoveryPhrase, mockUserAccounts } from './mock-data/user-accounts'
import { mockRPCResponse } from './mock-data/rpc-response'
import { CurrentPriceMockData } from './mock-data/current-price-data'
import { PriceHistoryMockData } from './mock-data/price-history-data'
import { mockUserWalletPreferences } from './mock-data/user-wallet-preferences'
import { formatePrices } from '../utils/format-prices'
import { BuyAssetUrl } from '../utils/buy-asset-url'
import locale from '../constants/locale'
import {
  HardwareWalletAccount,
  HardwareWalletConnectOpts
} from '../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
export default {
  title: 'Wallet/Desktop',
  argTypes: {
    onboarding: { control: { type: 'boolean', onboard: false } },
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const _DesktopWalletConcept = (args: { onboarding: boolean, locked: boolean }) => {
  const { onboarding, locked } = args
  const [view] = React.useState<NavTypes>('crypto')
  const [needsOnboarding, setNeedsOnboarding] = React.useState<boolean>(onboarding)
  const [walletLocked, setWalletLocked] = React.useState<boolean>(locked)
  const [needsBackup, setNeedsBackup] = React.useState<boolean>(true)
  const [showBackup, setShowBackup] = React.useState<boolean>(false)
  const [inputValue, setInputValue] = React.useState<string>('')
  const [hasRestoreError, setHasRestoreError] = React.useState<boolean>(false)
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)
  const [selectedTimeline, setSelectedTimeline] = React.useState<AssetPriceTimeframe>(AssetPriceTimeframe.OneDay)
  const [selectedAssetPriceHistory, setSelectedAssetPriceHistory] = React.useState<PriceDataObjectType[]>(PriceHistoryMockData.slice(15, 20))
  const [selectedAsset, setSelectedAsset] = React.useState<AssetOptionType>()
  const [selectedNetwork, setSelectedNetwork] = React.useState<Network>(Network.Mainnet)
  const [selectedAccount, setSelectedAccount] = React.useState<UserAccountType>(mockUserAccounts[0])
  const [showAddModal, setShowAddModal] = React.useState<boolean>(false)
  const [fromAsset, setFromAsset] = React.useState<AssetOptionType>(AssetOptions[0])
  const [toAsset, setToAsset] = React.useState<AssetOptionType>(AssetOptions[1])
  const [orderType, setOrderType] = React.useState<OrderTypes>('market')
  const [exchangeRate, setExchangeRate] = React.useState('0.0027533')
  const [slippageTolerance, setSlippageTolerance] = React.useState<SlippagePresetObjectType>(SlippagePresetOptions[0])
  const [orderExpiration, setOrderExpiration] = React.useState<ExpirationPresetObjectType>(ExpirationPresetOptions[0])
  const [toAddress, setToAddress] = React.useState('')
  const [buyAmount, setBuyAmount] = React.useState('')
  const [sendAmount, setSendAmount] = React.useState('')
  const [fromAmount, setFromAmount] = React.useState('')
  const [toAmount, setToAmount] = React.useState('')

  // In the future these will be actual paths
  // for example wallet/rewards
  // const navigateTo = (path: NavTypes) => {
  //   setView(path)
  // }

  const completeWalletSetup = (recoveryVerified: boolean) => {
    setNeedsOnboarding(false)
    setNeedsBackup(recoveryVerified)
  }

  const onWalletBackedUp = () => {
    setNeedsBackup(false)
  }

  const passwordProvided = (password: string) => {
    console.log('Password provided')
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

  const onUpdateAccountName = () => {
    alert('Will update account name')
  }

  const onShowBackup = () => {
    setShowBackup(true)
  }

  const onHideBackup = () => {
    setShowBackup(false)
  }

  const onRestore = (phrase: string, password: string) => {
    if (JSON.stringify(phrase.split(' ')) === JSON.stringify(recoveryPhrase)) {
      completeWalletSetup(true)
    } else {
      setHasRestoreError(true)
    }
  }

  const selectedUSDAssetPrice = React.useMemo(() => {
    if (selectedAsset) {
      const data = CurrentPriceMockData.find((coin) => coin.symbol === selectedAsset.symbol)
      const usdValue = data ? data.usd : '0'
      const usd24hChange = data ? data.usd24hChange : '0'
      const response: AssetPriceInfo = {
        price: usdValue,
        asset24hChange: usd24hChange,
        fromAsset: '',
        toAsset: ''
      }
      return response
    }
    return undefined
  }, [selectedAsset])

  const selectedBTCAssetPrice = React.useMemo(() => {
    if (selectedAsset) {
      const data = CurrentPriceMockData.find((coin) => coin.symbol === selectedAsset.symbol)
      const btcValue = data ? data.btc : '0'
      const btc24hChange = data ? data.btc24hChange : '0'
      const response: AssetPriceInfo = {
        price: btcValue,
        asset24hChange: btc24hChange,
        fromAsset: '',
        toAsset: ''
      }
      return response
    }
    return undefined
  }, [selectedAsset])

  // This returns info about a single asset
  const assetInfo = (account: RPCResponseType) => {
    return account.assets.find((a) => a.id === selectedAsset?.id)
  }

  // This calculates the fiat value of a single accounts asset balance
  const singleAccountFiatBalance = (account: RPCResponseType) => {
    const asset = assetInfo(account)
    const data = CurrentPriceMockData.find((coin) => coin.symbol === asset?.symbol)
    const value = data ? asset ? Number(asset.balance) * Number(data.usd) : 0 : 0
    return formatePrices(value)
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
    const id = selectedAsset ? selectedAsset.id : ''
    const list = selectedAsset ? mockRPCResponse.filter((account) => account.assets.map((assetID) => assetID.id).includes(id)) : mockRPCResponse
    const newList = list.map((wallet) => {
      const walletInfo = accountInfo(wallet)
      const id = walletInfo ? walletInfo.id : ''
      const name = walletInfo ? walletInfo.name : locale.account
      return {
        id: id,
        name: name,
        address: wallet.address,
        balance: Number(singleAccountBalance(wallet)),
        fiatBalance: singleAccountFiatBalance(wallet),
        asset: selectedAsset ? selectedAsset.symbol : '',
        accountType: 'Primary'
      }
    })
    return newList
  }, [selectedAsset, mockRPCResponse])

  // This returns a list of transactions from all accounts filtered by selected asset
  const transactions = React.useMemo(() => {
    const response = mockRPCResponse
    const transactionList = response.map((account) => {
      const id = selectedAsset ? selectedAsset.id : ''
      return account.transactions.find((item) => item.assetId === id)
    })
    const removedEmptyTransactions = transactionList.filter(x => x)
    return removedEmptyTransactions
  }, [selectedAsset])

  // This will scrape all of the user's accounts and combine the balances for a single asset
  const scrapedFullAssetBalance = (asset: AssetOptionType) => {
    const response = mockRPCResponse
    const amounts = response.map((account) => {
      const balance = account.assets.find((item) => item.id === asset.id)?.balance
      return balance ? balance : 0
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  // This will scrape all of the user's accounts and combine the fiat value for a single asset
  const scrapedFullAssetFiatBalance = (asset: AssetOptionType) => {
    const fullBallance = scrapedFullAssetBalance(asset)
    const price = Number(CurrentPriceMockData.find((coin) => coin.symbol === asset?.symbol)?.usd)
    const value = price ? price * fullBallance : 0
    return value
  }

  const userAssetList = React.useMemo(() => {
    const userList = mockUserWalletPreferences.viewableAssets
    const newList = AssetOptions.filter((asset) => userList.includes(asset.id))
    return newList.map((asset) => {
      return {
        asset: asset,
        assetBalance: scrapedFullAssetBalance(asset),
        fiatBalance: scrapedFullAssetFiatBalance(asset)
      }

    })
  }, [mockUserWalletPreferences.viewableAssets])

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const scrapedFullPortfolioBalance = () => {
    const amountList = userAssetList.map((item) => {
      return scrapedFullAssetFiatBalance(item.asset)
    })
    const grandTotal = amountList.reduce(function (a, b) {
      return a + b
    }, 0)
    return formatePrices(grandTotal)
  }

  // This will change once we hit a real api for pricing
  const timeline = (path: AssetPriceTimeframe) => {
    switch (path) {
      case AssetPriceTimeframe.Live:
        return 17
      case AssetPriceTimeframe.OneDay:
        return 15
      case AssetPriceTimeframe.OneWeek:
        return 12
      case AssetPriceTimeframe.OneMonth:
        return 10
      case AssetPriceTimeframe.ThreeMonths:
        return 8
      case AssetPriceTimeframe.OneYear:
        return 4
      case AssetPriceTimeframe.All:
        return 0
    }
  }

  // This updates the price chart timeline
  const onChangeTimeline = (path: AssetPriceTimeframe) => {
    setSelectedAssetPriceHistory(PriceHistoryMockData.slice(timeline(path), 20))
    setSelectedTimeline(path)
  }

  const onSelectAsset = (asset: AssetOptionType) => {
    setSelectedAsset(asset)
  }

  const onCreateAccount = (name: string) => {
    alert(name)
  }

  const onImportAccount = (name: string, key: string) => {
    alert(`Account Name: ${name}, Private Key: ${key}`)
  }

  const onToggleAddModal = () => {
    setShowAddModal(!showAddModal)
  }

  const onSelectNetwork = (network: Network) => {
    setSelectedNetwork(network)
  }

  const onUpdateWatchList = () => {
    alert('Will update Watchlist')
  }

  const onSelectTransactAsset = (asset: AssetOptionType, toOrFrom: ToOrFromType) => {
    if (toOrFrom === 'from') {
      setFromAsset(asset)
    } else {
      setToAsset(asset)
    }
  }

  const flipSwapAssets = () => {
    setFromAsset(toAsset)
    setToAsset(fromAsset)
  }

  const onSubmitBuy = (asset: AssetOptionType) => {
    const url = BuyAssetUrl(selectedNetwork, asset, selectedAccount, buyAmount)
    if (url) {
      window.open(url, '_blank')
    }
  }

  const onSubmitSwap = () => {
    alert('Submit Swap Transaction')
  }

  const onSubmitSend = () => {
    alert('Submit Send Transaction')
  }

  const calculateToAmount = (amount: number, market: boolean) => {
    if (market) {
      const calculated = Number(amount) / Number(exchangeRate)
      setToAmount(calculated.toString())
    } else {
      const calculated = Number(fromAmount) / Number(amount)
      setToAmount(calculated.toString())
    }
  }

  const onToggleOrderType = () => {
    if (orderType === 'market') {
      setOrderType('limit')
    } else {
      setOrderType('market')
      setExchangeRate('0.0027533')
      calculateToAmount(Number('0.0027533'), false)
    }
  }

  const fromAssetBalance = '26'
  const toAssetBalance = '78'

  const onSelectPresetAmount = (percent: number) => {
    const amount = Number(fromAssetBalance) * percent
    setFromAmount(amount.toString())
    calculateToAmount(amount, true)
  }

  const onSelectAccount = (account: UserAccountType) => {
    setSelectedAccount(account)
  }

  const onSelectExpiration = (expiration: ExpirationPresetObjectType) => {
    setOrderExpiration(expiration)
  }

  const onSelectSlippageTolerance = (slippage: SlippagePresetObjectType) => {
    setSlippageTolerance(slippage)
  }

  const onSetExchangeRate = (value: string) => {
    setExchangeRate(value)
    calculateToAmount(Number(value), false)
  }

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

  const onSetSendAmount = (value: string) => {
    setSendAmount(value)
  }

  const onSetFromAmount = (value: string) => {
    setFromAmount(value)
    calculateToAmount(Number(value), true)
  }

  const onSetToAmount = (value: string) => {
    setToAmount(value)
  }

  const onSetToAddress = (value: string) => {
    setToAddress(value)
  }

  const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Result.Type<HardwareWalletAccount[]> => {
    const makeDerivationPath = (index: number): string => `m/44'/60'/${index}'/0/0`

    return Array.from({ length: opts.stopIndex - opts.startIndex }, (_, i) => ({
      address: '0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2',
      derivationPath: makeDerivationPath(i + opts.startIndex),
      balance: '0.012345',
      ticker: 'ETH'
    }))
  }

  return (
    <WalletPageLayout>
      {/* <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      /> */}
      <WalletSubViewLayout>
        {needsOnboarding ?
          (
            <Onboarding
              recoveryPhrase={recoveryPhrase}
              onSubmit={completeWalletSetup}
              onPasswordProvided={passwordProvided}
              onRestore={onRestore}
              hasRestoreError={hasRestoreError}
            />
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
                    />
                  ) : (
                    <>
                      {showBackup ? (
                        <BackupWallet
                          isOnboarding={false}
                          onCancel={onHideBackup}
                          onSubmit={onWalletBackedUp}
                          recoveryPhrase={recoveryPhrase}
                        />
                      ) : (
                        <CryptoView
                          onLockWallet={lockWallet}
                          needsBackup={needsBackup}
                          onShowBackup={onShowBackup}
                          accounts={accounts}
                          onChangeTimeline={onChangeTimeline}
                          selectedAssetPriceHistory={selectedAssetPriceHistory}
                          selectedTimeline={selectedTimeline}
                          selectedAsset={selectedAsset}
                          onSelectAsset={onSelectAsset}
                          portfolioPriceHistory={selectedAssetPriceHistory}
                          portfolioBalance={scrapedFullPortfolioBalance()}
                          transactions={transactions}
                          selectedUSDAssetPrice={selectedUSDAssetPrice}
                          selectedBTCAssetPrice={selectedBTCAssetPrice}
                          userAssetList={userAssetList}
                          onCreateAccount={onCreateAccount}
                          onImportAccount={onImportAccount}
                          onConnectHardwareWallet={onConnectHardwareWallet}
                          isLoading={false}
                          showAddModal={showAddModal}
                          onToggleAddModal={onToggleAddModal}
                          onUpdateAccountName={onUpdateAccountName}
                          onUpdateWatchList={onUpdateWatchList}
                          userWatchList={['1']}
                          selectedNetwork={selectedNetwork}
                          onSelectNetwork={onSelectNetwork}
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
      </WalletSubViewLayout>
      {!needsOnboarding && !walletLocked &&
        <WalletWidgetStandIn>
          <BuySendSwap
            orderType={orderType}
            swapToAsset={toAsset}
            exchangeRate={exchangeRate}
            orderExpiration={orderExpiration}
            slippageTolerance={slippageTolerance}
            swapFromAsset={fromAsset}
            accounts={mockUserAccounts}
            selectedNetwork={selectedNetwork}
            selectedAccount={selectedAccount}
            buyAmount={buyAmount}
            sendAmount={sendAmount}
            fromAmount={fromAmount}
            toAmount={toAmount}
            fromAssetBalance={fromAssetBalance}
            toAssetBalance={toAssetBalance}
            toAddress={toAddress}
            onSubmitBuy={onSubmitBuy}
            onSetBuyAmount={onSetBuyAmount}
            onSetSendAmount={onSetSendAmount}
            onSetToAddress={onSetToAddress}
            onSetFromAmount={onSetFromAmount}
            onSetToAmount={onSetToAmount}
            onSubmitSend={onSubmitSend}
            onSubmitSwap={onSubmitSwap}
            flipSwapAssets={flipSwapAssets}
            onSelectNetwork={onSelectNetwork}
            onSelectAccount={onSelectAccount}
            onToggleOrderType={onToggleOrderType}
            onSelectAsset={onSelectTransactAsset}
            onSelectExpiration={onSelectExpiration}
            onSetExchangeRate={onSetExchangeRate}
            onSelectSlippageTolerance={onSelectSlippageTolerance}
            onSelectPresetAmount={onSelectPresetAmount}
          />
        </WalletWidgetStandIn>
      }
    </WalletPageLayout>
  )
}

_DesktopWalletConcept.args = {
  onboarding: false,
  locked: false
}

_DesktopWalletConcept.story = {
  name: 'Concept'
}
