import * as React from 'react'
import { useLocation } from 'react-router-dom'

// components
import { Image, LoadingOverlay, LoadIcon } from './nft-content-styles'
import { NftDetails } from '../nft-details/nft-details'

// utils
import { BraveWallet, NFTMetadataReturnType } from '../../../constants/types'

interface Props {
  isLoading?: boolean
  selectedAsset?: BraveWallet.BlockchainToken
  nftMetadata?: NFTMetadataReturnType
  tokenNetwork?: BraveWallet.NetworkInfo
}

export const NftContent = (props: Props) => {
  const { isLoading, selectedAsset } = props

  // hooks
  const { search } = useLocation()

  // memos
  const params = React.useMemo(() => {
    return new URLSearchParams(search)
  }, [search])

  const imageUrl = params.get('imageUrl')
  const imageWidth = params.get('imageWidth')
  const imageHeight = params.get('imageHeight')

  return (
    <>
      {imageUrl
        ? <Image
          customWidth={imageWidth ?? ''}
          customHeight={imageHeight ?? ''}
          src={imageUrl}
        />
        : <>
          {isLoading &&
            <LoadingOverlay isLoading={isLoading}>
              <LoadIcon />
            </LoadingOverlay>
          }
          {selectedAsset &&
            <NftDetails
              {...props}
              selectedAsset={selectedAsset}
            />
          }
        </>
      }
    </>
  )
}
