import * as React from 'react'
import { ERCToken } from '../../../constants/types'
import { IconWrapper, PlaceholderText } from './style'
import { stripERC20TokenImageURL } from '../../../utils/string-utils'
import { background } from 'ethereum-blockies'
import { ETH } from '../../../options/asset-options'

interface Config {
  size: 'big' | 'small'
  marginLeft?: number
  marginRight?: number
}

interface Props {
  selectedAsset?: ERCToken
}

function withPlaceholderIcon (WrappedComponent: React.ComponentType<any>, config: Config) {
  const {
    size,
    marginLeft,
    marginRight
  } = config

  return function (props: Props) {
    const { selectedAsset } = props

    if (!selectedAsset) {
      return null
    }

    const bg = React.useMemo(() => {
      if (selectedAsset?.symbol !== 'ETH' && stripERC20TokenImageURL(selectedAsset?.logo) === '') {
        return background({ seed: selectedAsset.contractAddress ? selectedAsset?.contractAddress.toLowerCase() : selectedAsset.name })
      }
    }, [selectedAsset])

    if (selectedAsset?.symbol !== 'ETH' && stripERC20TokenImageURL(selectedAsset?.logo) === '') {
      return (
        <IconWrapper
          panelBackground={bg}
          isPlaceholder={true}
          size={size}
          marginLeft={marginLeft ?? 0}
          marginRight={marginRight ?? 0}
        >
          <PlaceholderText size={size}>{selectedAsset?.symbol.charAt(0)}</PlaceholderText>
        </IconWrapper>
      )
    }
    return (
      <IconWrapper
        isPlaceholder={false}
        size={size}
        marginLeft={marginLeft ?? 0}
        marginRight={marginRight ?? 0}
      >
        <WrappedComponent icon={selectedAsset?.symbol === 'ETH' ? ETH.asset.logo : selectedAsset?.logo} />
      </IconWrapper>
    )

  }
}

export default withPlaceholderIcon
