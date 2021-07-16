import * as React from 'react'
import { AssetOptionType } from '../../../constants/types'
// Styled Components
import {
  StyledWrapper,
  AssetBalance,
  AssetAndBalance,
  AssetName,
  AssetIcon
} from './style'

export interface Props {
  asset: AssetOptionType
  onSelectAsset: () => void
}

function SelectAssetItem (props: Props) {
  const { asset, onSelectAsset } = props

  return (
    <StyledWrapper onClick={onSelectAsset}>
      <AssetIcon icon={asset.icon} />
      <AssetAndBalance>
        <AssetName>{asset.name}</AssetName>
        <AssetBalance>0 {asset.symbol}</AssetBalance>
      </AssetAndBalance>
    </StyledWrapper>
  )
}

export default SelectAssetItem
