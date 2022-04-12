import * as React from 'react'

import {
  BraveWallet,
  DefaultCurrencies,
  WalletRoutes,
  WalletAccountType
} from '../../../constants/types'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  AddAssetButton
} from './style'

import { PortfolioAssetItem } from '../../desktop'
import { useBalance } from '../../../common/hooks'

export interface Props {
  userAssetList: BraveWallet.BlockchainToken[]
  defaultCurrencies: DefaultCurrencies
  networkList: BraveWallet.NetworkInfo[]
  selectedAccount: WalletAccountType
  onAddAsset: () => void
}

const AssetsPanel = (props: Props) => {
  const {
    userAssetList,
    defaultCurrencies,
    selectedAccount,
    networkList,
    onAddAsset
  } = props

  const getBalance = useBalance(networkList)

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
      <AddAssetButton
        onClick={onAddAsset}
      >
        {getLocale('braveWalletAddAsset')}
      </AddAssetButton>
      {userAssetList?.map((asset) =>
        <PortfolioAssetItem
          defaultCurrencies={defaultCurrencies}
          action={onClickAsset(asset.symbol)}
          key={asset.contractAddress}
          assetBalance={getBalance(selectedAccount, asset)}
          token={asset}
          networks={networkList}
          isPanel={true}
        />
      )}
    </StyledWrapper>
  )
}

export default AssetsPanel
