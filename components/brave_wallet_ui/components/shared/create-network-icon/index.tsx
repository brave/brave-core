import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { IconWrapper, Placeholder, NetworkIcon } from './style'
import { stripERC20TokenImageURL, isRemoteImageURL, isValidIconExtension } from '../../../utils/string-utils'
import { create } from 'ethereum-blockies'
import {
  ETHIconUrl
} from '../../../assets/asset-icons'

interface Props {
  network: BraveWallet.EthereumChain
  marginRight: number
}

function CreateNetworkIcon (props: Props) {
  const { network, marginRight } = props
  if (!network) {
    return null
  }
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
  const testNetworkList = ['0x4', '0x3', '0x5', '0x2a', '0x539']
  const needsPlaceholder = !['0x1', ...testNetworkList].includes(network.chainId) && (networkImageURL === '' || !isValidIcon)

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
      <NetworkIcon icon={['0x1', ...testNetworkList].includes(network.chainId) ? ETHIconUrl : isRemoteURL ? remoteImage : network.iconUrls[0]} />
    </IconWrapper>
  )
}

export default CreateNetworkIcon
