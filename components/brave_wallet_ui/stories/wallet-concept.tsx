import * as React from 'react'
import { WalletWidgetStandIn } from './style'
import {
  SideNav,
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
  AssetPriceReturnInfo,
  RPCResponseType
} from '../constants/types'
import Onboarding from './screens/onboarding'
import BackupWallet from './screens/backup-wallet'
import { NavOptions } from '../options/side-nav-options'
import { AssetOptions } from '../options/asset-options'
import BuySendSwap from '../components/buy-send-swap'
import { recoveryPhrase, mockUserAccounts } from './mock-data/user-accounts'
import { mockRPCResponse } from './mock-data/rpc-response'
import { CurrentPriceMockData } from './mock-data/current-price-data'
import { PriceHistoryMockData } from './mock-data/price-history-data'
import { mockUserWalletPreferences } from './mock-data/user-wallet-preferences'
import { formatePrices } from '../utils/format-prices'
import locale from '../constants/locale'
export default {
  title: 'Wallet/Desktop',
  argTypes: {
    onboarding: { control: { type: 'boolean', onboard: false } },
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const _DesktopWalletConcept = (args: { onboarding: boolean, locked: boolean }) => {
  const { onboarding, locked } = args
  const [view, setView] = React.useState<NavTypes>('crypto')
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
  const [showAddModal, setShowAddModal] = React.useState<boolean>(false)

  // In the future these will be actual paths
  // for example wallet/rewards
  const navigateTo = (path: NavTypes) => {
    setView(path)
  }

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

  const selectedAssetPrice = React.useMemo(() => {
    if (selectedAsset) {
      const data = CurrentPriceMockData.find((coin) => coin.symbol === selectedAsset.symbol)
      const usdValue = data ? data.usd : 0
      const btcValue = data ? data.btc : 0
      const change24Hour = data ? data.change24Hour : 0
      const response: AssetPriceReturnInfo = {
        usd: formatePrices(usdValue),
        btc: btcValue,
        change24Hour: change24Hour
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
    const value = data ? asset ? asset.balance * data.usd : 0 : 0
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
    const price = CurrentPriceMockData.find((coin) => coin.symbol === asset?.symbol)?.usd
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

  const onConnectHardwareWallet = (hardware: 'Ledger' | 'Trezor') => {
    alert(`Connecting to ${hardware} wallet`)
  }

  const onToggleAddModal = () => {
    setShowAddModal(!showAddModal)
  }

  return (
    <WalletPageLayout>
      <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      />
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
                          selectedAssetPrice={selectedAssetPrice}
                          userAssetList={userAssetList}
                          onConnectHardwareWallet={onConnectHardwareWallet}
                          onCreateAccount={onCreateAccount}
                          onImportAccount={onImportAccount}
                          isLoading={false}
                          showAddModal={showAddModal}
                          onToggleAddModal={onToggleAddModal}
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
      <WalletWidgetStandIn>
        {!needsOnboarding && !walletLocked &&
          <BuySendSwap />
        }
      </WalletWidgetStandIn>
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
