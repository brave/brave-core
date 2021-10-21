import * as React from 'react'
import { Checkbox } from 'brave-ui'
import { TokenInfo } from '../../../constants/types'

// Options
import { ETH } from '../../../options/asset-options'

// Styled Components
import {
  StyledWrapper,
  AssetName,
  NameAndIcon,
  AssetIcon,
  DeleteButton,
  DeleteIcon,
  RightSide,
  NameAndSymbol,
  AssetSymbol
} from './style'

export interface Props {
  onSelectAsset: (key: string, selected: boolean, token: TokenInfo, isCustom: boolean) => void
  onRemoveAsset: (token: TokenInfo) => void
  isCustom: boolean
  isSelected: boolean
  token: TokenInfo
}

const AssetWatchlistItem = (props: Props) => {
  const {
    onSelectAsset,
    onRemoveAsset,
    isCustom,
    token,
    isSelected
  } = props

  const onCheck = (key: string, selected: boolean) => {
    onSelectAsset(key, selected, token, isCustom)
  }

  const onClickRemoveAsset = () => {
    onRemoveAsset(token)
  }

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AssetIcon icon={(token.symbol === 'ETH' ? ETH.asset.logo : token.logo) ?? ''} />
        <NameAndSymbol>
          <AssetName>{token.name}</AssetName>
          <AssetSymbol>{token.symbol}</AssetSymbol>
        </NameAndSymbol>
      </NameAndIcon>
      <RightSide>
        {isCustom &&
          <DeleteButton onClick={onClickRemoveAsset}>
            <DeleteIcon />
          </DeleteButton>
        }
        <Checkbox value={{ [token.contractAddress]: isSelected }} onChange={onCheck}>
          <div data-key={token.contractAddress} />
        </Checkbox>
      </RightSide>
    </StyledWrapper>
  )
}

export default AssetWatchlistItem
