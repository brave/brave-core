import * as React from 'react'

// Components
import {
  ConnectWithSite,
  ConnectedPanel,
  Panel,
  WelcomePanel,
  SignPanel,
  AllowSpendPanel,
  AllowAddNetworkPanel,
  ConfirmTransactionPanel
} from '../components/extension'
import { AppList } from '../components/shared'
import {
  Send,
  Buy,
  SelectAsset,
  SelectNetwork,
  SelectAccount
} from '../components/buy-send-swap'
import {
  WalletAccountType,
  PanelTypes,
  AppObjectType,
  AppsListType,
  AssetOptionType,
  BuySendSwapViewTypes,
  Network
} from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import { NetworkOptions } from '../options/network-options'
import { WyreAssetOptions } from '../options/wyre-asset-options'
import { filterAppList } from '../utils/filter-app-list'
import { BuyAssetUrl } from '../utils/buy-asset-url'
import LockPanel from '../components/extension/lock-panel'
import {
  StyledExtensionWrapper,
  ScrollContainer,
  SelectContainer
} from './style'
import { AssetOptions } from '../options/asset-options'

export default {
  title: 'Wallet/Extension/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
}

const accounts: WalletAccountType[] = [
  {
    id: '1',
    name: 'Account 1',
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    balance: '0.31178',
    asset: 'eth',
    fiatBalance: '0',
    accountType: 'Primary',
    tokens: []
  },
  {
    id: '2',
    name: 'Account 2',
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
    balance: '0.31178',
    asset: 'eth',
    fiatBalance: '0',
    accountType: 'Primary',
    tokens: []
  },
  {
    id: '3',
    name: 'Account 3',
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    balance: '0.31178',
    asset: 'eth',
    fiatBalance: '0',
    accountType: 'Primary',
    tokens: []
  }
]

const batTokenInfo = {
  contractAddress: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
  name: 'Basic Attention Token',
  isErc20: true,
  isErc721: false,
  symbol: 'BAT',
  decimals: 18,
  icon: ''
}

export const _ConfirmTransaction = () => {

  const transactionPanelPayload = {
    transactionAmount: '68000000000000000000',
    transactionGas: '7548000000000000',
    toAddress: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
    erc20Token: batTokenInfo,
    tokenPrice: '0.35',
    ethPrice: '3058.35'
  }

  const onConfirmTransaction = () => {
    alert('Confirmed Transaction')
  }

  const onRejectTransaction = () => {
    alert('Rejected Transaction')
  }

  const onClickMore = () => {
    alert('Will Show More Modal')
  }

  return (
    <StyledExtensionWrapper>
      <ConfirmTransactionPanel
        selectedNetwork={Network.Mainnet}
        onConfirm={onConfirmTransaction}
        onReject={onRejectTransaction}
        selectedAccount={accounts[0]}
        onClickMore={onClickMore}
        transactionPayload={transactionPanelPayload}
      />
    </StyledExtensionWrapper>
  )
}

_ConfirmTransaction.story = {
  name: 'Confirm Transaction'
}

export const _AllowAddNetwork = () => {

  const networkPayload = {
    siteUrl: 'https://app.compound.finance',
    contractAddress: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    chainInfo: {
      chainId: '',
      name: 'BSC (Binance Smart Chain)',
      url: 'https://bsc.binance.com'
    }
  }

  const onApprove = () => {
    alert('Approved Adding Network')
  }

  const onCancel = () => {
    alert('Canceled Adding Network')
  }

  return (
    <StyledExtensionWrapper>
      <AllowAddNetworkPanel
        selectedNetwork={Network.Mainnet}
        onApprove={onApprove}
        onCancel={onCancel}
        networkPayload={networkPayload}
      />
    </StyledExtensionWrapper>
  )
}

_AllowAddNetwork.story = {
  name: 'Allow Add Network'
}

export const _AllowSpend = () => {
  const spendPayload = {
    siteUrl: 'https://app.compound.finance',
    contractAddress: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    erc20Token: batTokenInfo,
    transactionFeeWei: '0.002447',
    transactionFeeFiat: '$6.57'
  }

  const onConfirm = () => {
    alert('Confirmed Spend')
  }

  const onReject = () => {
    alert('Rejected Spend')
  }

  return (
    <StyledExtensionWrapper>
      <AllowSpendPanel
        selectedNetwork={Network.Mainnet}
        onConfirm={onConfirm}
        onReject={onReject}
        spendPayload={spendPayload}
      />
    </StyledExtensionWrapper>
  )
}

_AllowSpend.story = {
  name: 'Allow Spend'
}

export const _SignTransaction = () => {

  const onSign = () => {
    alert('Signed Transaction')
  }

  const onCancel = () => {
    alert('Canceled Signing Transaction')
  }

  const onClickMore = () => {
    alert('Will Show More Modal')
  }

  return (
    <StyledExtensionWrapper>
      <SignPanel
        selectedAccount={accounts[0]}
        selectedNetwork={Network.Mainnet}
        message='To avoid digital cat burglars, sign below to authenticate with CryptoKitties.'
        onCancel={onCancel}
        onSign={onSign}
        onClickMore={onClickMore}
      />
    </StyledExtensionWrapper>
  )
}

_SignTransaction.story = {
  name: 'Sign Transaction'
}

export const _ConnectWithSite = () => {
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([
    accounts[0]
  ])
  const [readyToConnect, setReadyToConnect] = React.useState<boolean>(false)
  const url = 'https://app.uniswap.org'
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
    alert(`Connecting to ${url} using: ${JSON.stringify(selectedAccounts)}`)
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
    <StyledExtensionWrapper>
      <ConnectWithSite
        siteURL={url}
        isReady={readyToConnect}
        accounts={accounts}
        primaryAction={primaryAction}
        secondaryAction={secondaryAction}
        selectAccount={selectAccount}
        removeAccount={removeAccount}
        selectedAccounts={selectedAccounts}
      />
    </StyledExtensionWrapper>
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
    accounts[0]
  )
  const [favoriteApps, setFavoriteApps] = React.useState<AppObjectType[]>([
    AppsList[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [walletConnected, setWalletConnected] = React.useState<boolean>(true)
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)
  const [selectedNetwork, setSelectedNetwork] = React.useState<Network>(Network.Mainnet)
  const [selectedWyreAsset, setSelectedWyreAsset] = React.useState<AssetOptionType>(WyreAssetOptions[0])
  const [selectedAsset, setSelectedAsset] = React.useState<AssetOptionType>(AssetOptions[0])
  const [showSelectAsset, setShowSelectAsset] = React.useState<boolean>(false)
  const [toAddress, setToAddress] = React.useState('')
  const [fromAmount, setFromAmount] = React.useState('')
  const [buyAmount, setBuyAmount] = React.useState('')

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

  const onSubmitBuy = () => {
    const url = BuyAssetUrl(selectedNetwork, selectedWyreAsset, selectedAccount, buyAmount)
    if (url) {
      window.open(url, '_blank')
    }
  }

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    if (view === 'assets') {
      setShowSelectAsset(true)
    }
  }

  const onBack = () => {
    setSelectedPanel('main')
  }

  const onSelectNetwork = (network: Network) => () => {
    setSelectedNetwork(network)
    setSelectedPanel('main')
  }

  const onSelectAccount = (account: WalletAccountType) => () => {
    setSelectedAccount(account)
    setSelectedPanel('main')
  }

  const onHideSelectAsset = () => {
    setShowSelectAsset(false)
  }

  const onSelectAsset = (asset: AssetOptionType) => () => {
    if (selectedPanel === 'buy') {
      setSelectedWyreAsset(asset)
    } else {
      setSelectedAsset(asset)
    }
    setShowSelectAsset(false)
  }

  const onInputChange = (value: string, name: string) => {
    if (name === 'address') {
      setToAddress(value)
    } else {
      setFromAmount(value)
    }
  }

  const toggleConnected = () => {
    setWalletConnected(!walletConnected)
  }

  const getTitle = (path: PanelTypes) => {
    if (path === 'networks') {
      setPanelTitle('Select Network')
    } else {
      setPanelTitle(path)
    }
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

  const addToFavorites = (app: AppObjectType) => {
    const newList = [...favoriteApps, app]
    setFavoriteApps(newList)
  }
  const removeFromFavorites = (app: AppObjectType) => {
    const newList = favoriteApps.filter(
      (fav) => fav.name !== app.name
    )
    setFavoriteApps(newList)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList, setFilteredAppsList)
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

  const onSelectPresetAmount = (percent: number) => {
    const amount = Number(selectedAccount.balance) * percent
    setFromAmount(amount.toString())
  }

  const handlePasswordChanged = (value: string) => {
    setHasPasswordError(false)
    setInputValue(value)
  }

  const onSubmitSend = () => {
    alert('Will submit Send')
  }

  const onOpenSettings = () => {
    alert('Will go to Wallet Settings')
  }

  return (
    <StyledExtensionWrapper>
      {walletLocked ? (
        <LockPanel
          hasPasswordError={hasPasswordError}
          onSubmit={unlockWallet}
          disabled={inputValue === ''}
          onPasswordChanged={handlePasswordChanged}
        />
      ) : (
        <>
          {selectedPanel === 'main' ? (
            <ConnectedPanel
              selectedNetwork={selectedNetwork}
              selectedAccount={selectedAccount}
              isConnected={walletConnected}
              connectAction={toggleConnected}
              navAction={navigateTo}
              onLockWallet={onLockWallet}
              onOpenSettings={onOpenSettings}
            />
          ) : (
            <>
              {showSelectAsset &&
                <SelectContainer>
                  <SelectAsset
                    assets={AssetOptions}
                    onSelectAsset={onSelectAsset}
                    onBack={onHideSelectAsset}
                  />
                </SelectContainer>
              }
              {selectedPanel === 'accounts' &&
                <SelectContainer>
                  <SelectAccount
                    accounts={accounts}
                    onBack={onBack}
                    onSelectAccount={onSelectAccount}
                  />
                </SelectContainer>
              }
              {selectedPanel === 'networks' &&
                <SelectContainer>
                  <SelectNetwork
                    networks={NetworkOptions}
                    onBack={onBack}
                    onSelectNetwork={onSelectNetwork}
                  />
                </SelectContainer>
              }
              {!showSelectAsset && selectedPanel !== 'networks' && selectedPanel !== 'accounts' &&
                <Panel
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
                        onInputChange={onInputChange}
                        onSelectPresetAmount={onSelectPresetAmount}
                        onSubmit={onSubmitSend}
                        selectedAsset={selectedAsset}
                        selectedAssetAmount={fromAmount}
                        selectedAssetBalance={selectedAccount.balance.toString()}
                        toAddress={toAddress}
                      />
                    }
                    {selectedPanel === 'buy' &&
                      <Buy
                        onChangeBuyView={onChangeSendView}
                        onInputChange={onSetBuyAmount}
                        onSubmit={onSubmitBuy}
                        selectedAsset={selectedWyreAsset}
                        buyAmount={buyAmount}
                        selectedNetwork={selectedNetwork}
                      />
                    }
                  </ScrollContainer>
                </Panel>
              }
            </>
          )}
        </>
      )}
    </StyledExtensionWrapper>
  )
}

_ConnectedPanel.args = {
  locked: false
}

_ConnectedPanel.story = {
  name: 'Connected With Site'
}

export const _SetupWallet = () => {

  const onRestore = () => {
    alert('Will navigate to full wallet restore page')
  }

  const onSetup = () => {
    alert('Will navigate to full wallet onboarding page')
  }

  return (
    <StyledExtensionWrapper>
      <WelcomePanel onRestore={onRestore} onSetup={onSetup} />
    </StyledExtensionWrapper>
  )
}

_SetupWallet.story = {
  name: 'Setup New Wallet'
}
