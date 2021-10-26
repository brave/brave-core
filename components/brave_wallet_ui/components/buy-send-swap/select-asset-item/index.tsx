import * as React from 'react'
import { AccountAssetOptionType } from '../../../constants/types'
import { withPlaceholderIcon } from '../../shared'
// Styled Components
import {
  StyledWrapper,
  AssetBalance,
  AssetAndBalance,
  AssetName,
  AssetIcon
} from './style'

export interface Props {
  asset: AccountAssetOptionType
  onSelectAsset: () => void
}

function SelectAssetItem (props: Props) {
  const { asset, onSelectAsset } = props

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'small', marginLeft: 0, marginRight: 8 })
  }, [])

  return (
    <StyledWrapper onClick={onSelectAsset}>
      <AssetIconWithPlaceholder selectedAsset={asset.asset} />
      <AssetAndBalance>
        <AssetName>{asset.asset.name}</AssetName>
        <AssetBalance>{asset.asset.symbol}</AssetBalance>
      </AssetAndBalance>
    </StyledWrapper>
  )
}

export default SelectAssetItem
