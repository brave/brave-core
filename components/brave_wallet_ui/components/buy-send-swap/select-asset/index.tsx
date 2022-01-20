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

  const fuse = React.useMemo(() => new Fuse(assets, {
    shouldSort: true,
    threshold: 0.45,
    location: 0,
    distance: 100,
    minMatchCharLength: 1,
    keys: [
      { name: 'asset.name', weight: 0.5 },
      { name: 'asset.symbol', weight: 0.5 }
    ]
  }), [assets])

  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>(assets)

  const filterAssetList = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event.target.value
    if (search === '') {
      setFilteredAssetList(assets)
    } else {
      const filteredList = fuse.search(search).map((result: Fuse.FuseResult<BraveWallet.BlockchainToken>) => result.item)
      setFilteredAssetList(filteredList)
    }
  }, [fuse, assets])

  const erc271Tokens = React.useMemo(() => filteredAssetList.filter((token) => token.isErc721), [filteredAssetList])

  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectAsset')}
        onBack={onBack}
        hasAddButton={true}
        onClickAdd={onAddAsset}
      />
      <SearchBar placeholder={getLocale('braveWalletSearchAsset')} action={filterAssetList} autoFocus={true} />
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
