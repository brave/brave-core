import * as React from 'react'
import { Checkbox } from 'brave-ui'

// Styled Components
import {
  StyledWrapper,
  AssetName,
  NameAndIcon,
  AssetIcon,
  Balance,
  CheckboxRow
} from './style'

export interface Props {
  onSelectAsset: (key: string, selected: boolean) => void
  isSelected: boolean
  id: string
  name: string
  symbol: string
  icon?: string
  assetBalance: string
}

const AssetWatchlistItem = (props: Props) => {
  const {
    name,
    assetBalance,
    icon,
    symbol,
    id,
    onSelectAsset,
    isSelected
  } = props

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AssetIcon icon={icon ? icon : ''} />
        <AssetName>{name}</AssetName>
      </NameAndIcon>
      <Balance>{Number(assetBalance).toFixed(6)} {symbol}</Balance>
      <CheckboxRow>
        <Checkbox value={{ [id]: isSelected }} onChange={onSelectAsset}>
          <div data-key={id} />
        </Checkbox>
      </CheckboxRow>
    </StyledWrapper>
  )
}

export default AssetWatchlistItem
