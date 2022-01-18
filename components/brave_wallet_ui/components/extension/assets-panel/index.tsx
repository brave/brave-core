import * as React from 'react'

import {
  BraveWallet,
  DefaultCurrencies,
  WalletRoutes,
  WalletAccountType
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper
} from './style'

import { PortfolioAssetItem } from '../../desktop'
import { useBalance } from '../../../common/hooks'

export interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  userAssetList: BraveWallet.BlockchainToken[]
  defaultCurrencies: DefaultCurrencies
  selectedNetwork: BraveWallet.EthereumChain
  selectedAccount: WalletAccountType
}

const AssetsPanel = (props: Props) => {
  const {
    userAssetList,
    spotPrices,
    defaultCurrencies,
    selectedNetwork,
    selectedAccount
  } = props

  const getBalance = useBalance(selectedNetwork)

  const onClickAsset = (symbol: string) => () => {
    const url = `brave://wallet${WalletRoutes.Portfolio}/${symbol}`
    chrome.tabs.create({ url: url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  return (
    <StyledWrapper>
      {userAssetList?.map((asset) =>
        <PortfolioAssetItem
          spotPrices={spotPrices}
          defaultCurrencies={defaultCurrencies}
          action={onClickAsset(asset.symbol)}
          key={asset.contractAddress}
          assetBalance={getBalance(selectedAccount, asset)}
          token={asset}
        />
      )}
    </StyledWrapper>
  )
}

export default AssetsPanel
