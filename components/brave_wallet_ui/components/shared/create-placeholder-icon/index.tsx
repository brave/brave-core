import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { IconWrapper, PlaceholderText } from './style'
import { stripERC20TokenImageURL, isRemoteImageURL, isValidIconExtension } from '../../../utils/string-utils'
import { background } from 'ethereum-blockies'
import { ETH } from '../../../options/asset-options'

interface Config {
  size: 'big' | 'small'
  marginLeft?: number
  marginRight?: number
}

interface Props {
  selectedAsset?: BraveWallet.BlockchainToken
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
    const tokenImageURL = stripERC20TokenImageURL(selectedAsset?.logo)
    const isRemoteURL = isRemoteImageURL(tokenImageURL)
    const isDataURL = selectedAsset?.logo.startsWith('chrome://erc-token-images/')
    const isStorybook = selectedAsset?.logo.startsWith('static/media/components/brave_wallet_ui/')

    const isValidIcon = React.useMemo(() => {
      console.log(selectedAsset?.logo)
      if (isRemoteURL || isDataURL) {
        const url = new URL(selectedAsset?.logo)
        return isValidIconExtension(url.pathname)
      }
      if (isStorybook) {
        return true
      }
      return false
    }, [isRemoteURL, isDataURL, tokenImageURL])

    const needsPlaceholder = selectedAsset?.symbol !== 'ETH' && (tokenImageURL === '' || !isValidIcon)

    const bg = React.useMemo(() => {
      if (needsPlaceholder) {
        return background({ seed: selectedAsset.contractAddress ? selectedAsset?.contractAddress.toLowerCase() : selectedAsset.name })
      }
    }, [selectedAsset])

    const remoteImage = React.useMemo(() => {
      if (isRemoteURL) {
        return `chrome://image?${tokenImageURL}`
      }
      return ''
    }, [tokenImageURL])

    if (needsPlaceholder) {
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
        <WrappedComponent icon={selectedAsset?.symbol === 'ETH' ? ETH.logo : isRemoteURL ? remoteImage : selectedAsset?.logo} />
      </IconWrapper>
    )
  }
}

export default withPlaceholderIcon
