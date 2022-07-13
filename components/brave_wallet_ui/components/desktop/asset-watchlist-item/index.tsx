import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getTokensNetwork } from '../../../utils/network-utils'
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Components
import { withPlaceholderIcon } from '../../shared'
import { Checkbox } from 'brave-ui'

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
import { NFTAssetIcon } from '../portfolio-asset-item/style'

export interface Props {
  onSelectAsset: (key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => void
  onRemoveAsset: (token: BraveWallet.BlockchainToken) => void
  isCustom: boolean
  isSelected: boolean
  token: BraveWallet.BlockchainToken
  networkList: BraveWallet.NetworkInfo[]
}

const AssetWatchlistItem = (props: Props) => {
  const {
    onSelectAsset,
    onRemoveAsset,
    isCustom,
    token,
    isSelected,
    networkList
  } = props

  const onCheck = (key: string, selected: boolean) => {
    onSelectAsset(key, selected, token, isCustom)
  }

  const onClickAsset = () => {
    onSelectAsset(token.contractAddress, !isSelected, token, isCustom)
  }

  const onClickRemoveAsset = () => {
    onRemoveAsset(token)
  }

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(token.isErc721 ? NFTAssetIcon : AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [token])

  const tokensNetwork = React.useMemo(() => {
    if (!token) {
      return
    }
    return getTokensNetwork(networkList, token)
  }, [token, networkList])

  const networkDescription = React.useMemo(() => {
    return getLocale('braveWalletPortfolioAssetNetworkDescription')
      .replace('$1', token.symbol)
      .replace('$2', tokensNetwork?.chainName ?? '')
  }, [tokensNetwork, token])

  return (
    <StyledWrapper>
      <NameAndIcon onClick={onClickAsset}>
        <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
        <NameAndSymbol>
          <AssetName>
            {token.name} {
              token.isErc721 && token.tokenId
                ? '#' + new Amount(token.tokenId).toNumber()
                : ''
            }
          </AssetName>
          <AssetSymbol>{networkDescription}</AssetSymbol>
        </NameAndSymbol>
      </NameAndIcon>
      <RightSide>
        {isCustom &&
          <DeleteButton onClick={onClickRemoveAsset}>
            <DeleteIcon />
          </DeleteButton>
        }
        <Checkbox value={{ [`${token.contractAddress}-${token.symbol}-${token.chainId}-${token.tokenId}`]: isSelected }} onChange={onCheck}>
          <div data-key={`${token.contractAddress}-${token.symbol}-${token.chainId}-${token.tokenId}`} />
        </Checkbox>
      </RightSide>
    </StyledWrapper>
  )
}

export default AssetWatchlistItem
