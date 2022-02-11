import * as React from 'react'
import Fuse from 'fuse.js'

import SelectAssetItem from '../select-asset-item'
import { BraveWallet } from '../../../constants/types'
import { SearchBar } from '../../shared'
import Header from '../select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollSearchContainer,
  DivderTextWrapper,
  DividerText
} from '../shared-styles'
import useFuse from '../../../common/hooks/fuse'

export interface Props {
  assets: BraveWallet.BlockchainToken[]
  onAddAsset: () => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken) => () => void
  onBack: () => void
}

function SelectAsset (props: Props) {
  const {
    assets,
    onAddAsset,
    onBack,
    onSelectAsset
  } = props

  const fuseOptions: Fuse.IFuseOptions<BraveWallet.BlockchainToken> = React.useMemo(() => ({
    shouldSort: true,
    threshold: 0.45,
    location: 0,
    distance: 100,
    minMatchCharLength: 1,
    keys: [
      { name: 'name', weight: 0.5 },
      { name: 'symbol', weight: 0.5 }
    ]
  }), [])

  const [searchTerm, setSearchTerm] = React.useState('')

  const filteredAssetList = useFuse(assets, searchTerm, fuseOptions)
  const searchAction = (event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchTerm(event.target.value)
  }

  const erc271Tokens = React.useMemo(() => filteredAssetList.filter((token) => token.isErc721), [filteredAssetList])

  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectAsset')}
        onBack={onBack}
        hasAddButton={true}
        onClickAdd={onAddAsset}
      />
      <SearchBar placeholder={getLocale('braveWalletSearchAsset')} action={searchAction} autoFocus={true} />
      <SelectScrollSearchContainer>
        {
          // Temp filtering out erc721 tokens, sending will be handled in a different PR
          filteredAssetList.filter((token) => !token.isErc721).map((asset: BraveWallet.BlockchainToken) =>
            <SelectAssetItem
              key={asset.contractAddress}
              asset={asset}
              onSelectAsset={onSelectAsset(asset)}
            />
          )
        }
        {erc271Tokens.length > 0 &&
          <>
            <DivderTextWrapper>
              <DividerText>{getLocale('braveWalletTopNavNFTS')}</DividerText>
            </DivderTextWrapper>
            {erc271Tokens.map((asset: BraveWallet.BlockchainToken) =>
              <SelectAssetItem
                key={asset.contractAddress}
                asset={asset}
                onSelectAsset={onSelectAsset(asset)}
              />
            )
            }
          </>
        }
      </SelectScrollSearchContainer>
    </SelectWrapper>
  )
}

export default SelectAsset
