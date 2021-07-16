import * as React from 'react'
import {
  BuySendSwapTypes,
  UserAccountType,
  NetworkOptionsType,
  AssetOptionType,
  OrderTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType
} from '../../constants/types'
import Swap from '../../components/buy-send-swap/tabs/swap-tab'
import {
  Layout
} from '../../components/buy-send-swap'

export interface Props {
  accounts: UserAccountType[]
  orderType: OrderTypes
  swapToAsset: AssetOptionType
  swapFromAsset: AssetOptionType
  selectedNetwork: NetworkOptionsType
  selectedAccount: UserAccountType
  exchangeRate: string
  slippageTolerance: SlippagePresetObjectType
  orderExpiration: ExpirationPresetObjectType
  fromAmount: string
  toAmount: string
  fromAssetBalance: string
  toAssetBalance: string
  onSubmitSwap: () => void
  flipSwapAssets: () => void
  onSelectNetwork: (network: NetworkOptionsType) => void
  onSelectAccount: (account: UserAccountType) => void
  onToggleOrderType: () => void
  onSelectSwapAsset: (asset: AssetOptionType, toOrFrom: string) => void
  onSelectSlippageTolerance: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration: (expiration: ExpirationPresetObjectType) => void
  onSetExchangeRate: (value: string) => void
  onSetFromAmount: (value: string) => void
  onSetToAmount: (value: string) => void
  onSelectPresetAmount: (percent: number) => void
}

function BuySendSwap (props: Props) {
  const {
    accounts,
    orderType,
    swapToAsset,
    swapFromAsset,
    selectedNetwork,
    selectedAccount,
    exchangeRate,
    slippageTolerance,
    orderExpiration,
    fromAmount,
    toAmount,
    fromAssetBalance,
    toAssetBalance,
    onSubmitSwap,
    flipSwapAssets,
    onSelectNetwork,
    onSelectAccount,
    onToggleOrderType,
    onSelectSwapAsset,
    onSelectSlippageTolerance,
    onSelectExpiration,
    onSetExchangeRate,
    onSetFromAmount,
    onSetToAmount,
    onSelectPresetAmount
  } = props
  const [selectedTab, setSelectedTab] = React.useState<BuySendSwapTypes>('swap')

  const changeTab = (tab: BuySendSwapTypes) => () => {
    setSelectedTab(tab)
  }

  return (
    <Layout selectedTab={selectedTab} onChangeTab={changeTab}>
      {selectedTab === 'swap' &&
        <Swap
          accounts={accounts}
          orderType={orderType}
          swapToAsset={swapToAsset}
          swapFromAsset={swapFromAsset}
          selectedNetwork={selectedNetwork}
          selectedAccount={selectedAccount}
          exchangeRate={exchangeRate}
          orderExpiration={orderExpiration}
          slippageTolerance={slippageTolerance}
          fromAmount={fromAmount}
          toAmount={toAmount}
          fromAssetBalance={fromAssetBalance}
          toAssetBalance={toAssetBalance}
          onSubmitSwap={onSubmitSwap}
          flipSwapAssets={flipSwapAssets}
          onSelectNetwork={onSelectNetwork}
          onSelectAccount={onSelectAccount}
          onSelectSwapAsset={onSelectSwapAsset}
          onToggleOrderType={onToggleOrderType}
          onSelectSlippageTolerance={onSelectSlippageTolerance}
          onSelectExpiration={onSelectExpiration}
          onSetExchangeRate={onSetExchangeRate}
          onSetFromAmount={onSetFromAmount}
          onSetToAmount={onSetToAmount}
          onSelectPresetAmount={onSelectPresetAmount}
        />
      }
    </Layout>
  )
}

export default BuySendSwap
