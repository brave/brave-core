import * as React from 'react'
import { Checkbox } from 'brave-ui'
import { ERCToken } from '../../../constants/types'
import { withPlaceholderIcon } from '../../shared'
import { hexToNumber } from '../../../utils/format-balances'

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
  onSelectAsset: (key: string, selected: boolean, token: ERCToken, isCustom: boolean) => void
  onRemoveAsset: (token: ERCToken) => void
  isCustom: boolean
  isSelected: boolean
  token: ERCToken
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

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AssetIconWithPlaceholder selectedAsset={token} />
        <NameAndSymbol>
          <AssetName>{token.name} {token.isErc721 ? hexToNumber(token.tokenId ?? '') : ''}</AssetName>
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
