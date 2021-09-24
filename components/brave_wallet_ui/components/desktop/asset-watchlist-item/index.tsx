import * as React from 'react'
import { Checkbox } from 'brave-ui'
import { TokenInfo } from '../../../constants/types'
// Styled Components
import {
  StyledWrapper,
  AssetName,
  NameAndIcon,
  AssetIcon,
  DeleteButton,
  DeleteIcon,
  RightSide
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
        <AssetIcon icon={token.icon ? token.icon : ''} />
        <AssetName>{token.name}</AssetName>
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
