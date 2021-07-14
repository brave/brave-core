import * as React from 'react'
import SelectAssetItem from '../select-asset-item'
import { AssetOptionType } from '../../../constants/types'
import { SearchBar } from '../../shared'
import Header from '../select-header'
import locale from '../../../constants/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollSearchContainer
} from '../shared-styles'

export interface Props {
  assets: AssetOptionType[]
  onSelectAsset: (asset: AssetOptionType) => () => void
  onBack: () => void
}

function SelectAsset (props: Props) {
  const { assets, onBack, onSelectAsset } = props
  const [filteredAssetList, setFilteredAssetList] = React.useState<AssetOptionType[]>(assets)

  const filterAssetList = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setFilteredAssetList(assets)
    } else {
      const filteredList = assets.filter((item) => {
        return (
          item.name.toLowerCase() === search.toLowerCase() ||
          item.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredAssetList(filteredList)
    }
  }

  return (
    <SelectWrapper>
      <Header title={locale.selectAsset} onBack={onBack} />
      <SearchBar placeholder={locale.searchAsset} action={filterAssetList} />
      <SelectScrollSearchContainer>
        {filteredAssetList.map((asset) => <SelectAssetItem key={asset.id} asset={asset} onSelectAsset={onSelectAsset(asset)} />)}
      </SelectScrollSearchContainer>
    </SelectWrapper>
  )
}

export default SelectAsset
