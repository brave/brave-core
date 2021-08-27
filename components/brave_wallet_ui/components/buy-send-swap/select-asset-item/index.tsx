import * as React from 'react'
import { AccountAssetOptionType } from '../../../constants/types'
// Styled Components
import {
  StyledWrapper,
  AssetBalance,
  AssetAndBalance,
  AssetName,
  AssetIcon
} from './style'

import { formatBalance } from '../../../utils/format-balances'

export interface Props {
  asset: AccountAssetOptionType
  onSelectAsset: () => void
}

function SelectAssetItem (props: Props) {
  const { asset, onSelectAsset } = props

  return (
    <StyledWrapper onClick={onSelectAsset}>
      <AssetIcon icon={asset.asset.icon} />
      <AssetAndBalance>
        <AssetName>{asset.asset.name}</AssetName>
        <AssetBalance>{formatBalance(asset.assetBalance, asset.asset.decimals)} {asset.asset.symbol}</AssetBalance>
      </AssetAndBalance>
    </StyledWrapper>
  )
}

export default SelectAssetItem
