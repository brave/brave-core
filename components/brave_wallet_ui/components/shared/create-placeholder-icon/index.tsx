import * as React from 'react'
import { background } from 'ethereum-blockies'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import { stripERC20TokenImageURL, isRemoteImageURL, isValidIconExtension, httpifyIpfsUrl } from '../../../utils/string-utils'

// Styled components
import { IconWrapper, PlaceholderText } from './style'

// Options
import { makeNetworkAsset } from '../../../options/asset-options'

interface Config {
  size: 'big' | 'small'
  marginLeft?: number
  marginRight?: number
}

interface Props {
  asset: BraveWallet.BlockchainToken | undefined
  network: BraveWallet.NetworkInfo | undefined
}

function withPlaceholderIcon (WrappedComponent: React.ComponentType<any>, config: Config) {
  const {
    size,
    marginLeft,
    marginRight
  } = config

  return function (props: Props) {
    const { asset, network } = props

    if (!asset || !network) {
      return null
    }

    const nativeAsset = React.useMemo(
      () => makeNetworkAsset(network),
      [network]
    )

    const isNativeAsset = React.useMemo(() =>
      asset.symbol.toLowerCase() === nativeAsset.symbol.toLowerCase(),
      [nativeAsset, asset]
    )

    const tokenImageURL = stripERC20TokenImageURL(asset.logo)
    const isRemoteURL = isRemoteImageURL(tokenImageURL)
    const isDataURL = asset.logo.startsWith('chrome://erc-token-images/')
    const isStorybook = asset.logo.startsWith('static/media/components/brave_wallet_ui/')

    const isValidIcon = React.useMemo(() => {
      if (isRemoteURL || isDataURL) {
        return tokenImageURL?.includes('data:image/') ? true : isValidIconExtension(new URL(asset.logo).pathname)
      }
      if (isStorybook) {
        return true
      }
      return false
    }, [isRemoteURL, isDataURL, tokenImageURL])

    const needsPlaceholder = isNativeAsset
      ? (tokenImageURL === '' || !isValidIcon) && nativeAsset.logo === ''
      : tokenImageURL === '' || !isValidIcon

    const bg = React.useMemo(() => {
      if (needsPlaceholder) {
        return background({ seed: asset.contractAddress ? asset.contractAddress.toLowerCase() : asset.name })
      }
    }, [asset])

    const remoteImage = React.useMemo(() => {
      if (isRemoteURL) {
        return httpifyIpfsUrl(tokenImageURL)
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
          <PlaceholderText size={size}>{asset.symbol.charAt(0)}</PlaceholderText>
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
        <WrappedComponent
          icon={
            isNativeAsset && nativeAsset.logo
              ? nativeAsset.logo
              : isRemoteURL ? remoteImage : asset.logo
          }
        />
      </IconWrapper>
    )
  }
}

export default withPlaceholderIcon
