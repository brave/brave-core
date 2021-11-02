import * as React from 'react'
import { AccountAssetOptionType } from '../../../constants/types'
import { withPlaceholderIcon } from '../../shared'
import { hexToNumber } from '../../../utils/format-balances'

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
        <AssetName>{asset.asset.name} {asset.asset.isErc721 ? hexToNumber(asset.asset.tokenId ?? '') : ''}</AssetName>
        <AssetBalance>{asset.asset.symbol}</AssetBalance>
      </AssetAndBalance>
    </StyledWrapper>
  )
}

export default SelectAssetItem
