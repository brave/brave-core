import * as React from 'react'
import { create } from 'ethereum-blockies'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import { stripERC20TokenImageURL, isRemoteImageURL, isValidIconExtension } from '../../../utils/string-utils'

// Styled components
import { IconWrapper, Placeholder, NetworkIcon } from './style'

// Options
import { makeNetworkAsset } from '../../../options/asset-options'

interface Props {
  network: BraveWallet.NetworkInfo
  marginRight: number
}

function CreateNetworkIcon (props: Props) {
  const { network, marginRight } = props
  if (!network) {
    return null
  }

  const nativeAsset = React.useMemo(
    () => makeNetworkAsset(network),
    [network]
  )

  const networkImageURL = stripERC20TokenImageURL(network?.iconUrls[0])
  const isRemoteURL = isRemoteImageURL(networkImageURL)
  const isDataURL = network.iconUrls[0]?.startsWith('chrome://erc-token-images/')
  const isStorybook = network.iconUrls[0]?.startsWith('static/media/components/brave_wallet_ui/')

  const isValidIcon = React.useMemo(() => {
    if (isRemoteURL || isDataURL) {
      const url = new URL(network.iconUrls[0])
      return isValidIconExtension(url.pathname)
    }

    if (isStorybook) {
      return true
    }
    return false
  }, [isRemoteURL, isDataURL, networkImageURL])

  // Will remove these hardcoded chainId's.
  // We need to return a normal Ethereum Icon URL for Ethereum Mainnet
  // and return a grayed out Ethereum Icon Url for Ethereum Test Networks from the backend.
  const testNetworkList = [
    BraveWallet.RINKEBY_CHAIN_ID,
    BraveWallet.ROPSTEN_CHAIN_ID,
    BraveWallet.GOERLI_CHAIN_ID,
    BraveWallet.KOVAN_CHAIN_ID,
    BraveWallet.LOCALHOST_CHAIN_ID
  ]
  const needsPlaceholder = nativeAsset.logo === '' && (networkImageURL === '' || !isValidIcon)

  const orb = React.useMemo(() => {
    if (needsPlaceholder) {
      return create({ seed: network.chainName, size: 8, scale: 16 }).toDataURL()
    }
  }, [network])

  const remoteImage = React.useMemo(() => {
    if (isRemoteURL) {
      return `chrome://image?${networkImageURL}`
    }
    return ''
  }, [networkImageURL])

  if (needsPlaceholder) {
    return (
      <IconWrapper
        marginRight={marginRight ?? 0}
        isTestnet={false}
      >
        <Placeholder
          orb={orb}
        />
      </IconWrapper>
    )
  }
  return (
    <IconWrapper
      marginRight={marginRight ?? 0}
      isTestnet={testNetworkList.includes(network.chainId)}
    >
      <NetworkIcon
        icon={
          nativeAsset.logo
            ? nativeAsset.logo
            : isRemoteURL ? remoteImage : network.iconUrls[0]
        }
      />
    </IconWrapper>
  )
}

export default CreateNetworkIcon
