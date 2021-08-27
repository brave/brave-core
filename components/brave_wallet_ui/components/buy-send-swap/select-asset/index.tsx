import * as React from 'react'
import SelectAssetItem from '../select-asset-item'
import { AccountAssetOptionType } from '../../../constants/types'
import { SearchBar } from '../../shared'
import Header from '../select-header'
import locale from '../../../constants/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollSearchContainer
} from '../shared-styles'

export interface Props {
  assets: AccountAssetOptionType[]
  onSelectAsset: (asset: AccountAssetOptionType) => () => void
  onBack: () => void
}

function SelectAsset (props: Props) {
  const { assets, onBack, onSelectAsset } = props
  const [filteredAssetList, setFilteredAssetList] = React.useState<AccountAssetOptionType[]>(assets)

  const filterAssetList = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setFilteredAssetList(assets)
    } else {
      const filteredList = assets.filter((item) => {
        return (
          item.asset.name.toLowerCase() === search.toLowerCase() ||
          item.asset.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.asset.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.asset.symbol.toLowerCase().startsWith(search.toLowerCase())
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
        {filteredAssetList.map((asset) => <SelectAssetItem key={asset.asset.contractAddress} asset={asset} onSelectAsset={onSelectAsset(asset)} />)}
      </SelectScrollSearchContainer>
    </SelectWrapper>
  )
}

export default SelectAsset
