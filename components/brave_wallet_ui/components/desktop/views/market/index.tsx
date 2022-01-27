import * as React from 'react'

import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown } from '../..'
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'

import {
  StyledWrapper,
  TopRow
} from './style'

export interface Props {
}

const MarketView = (props: Props) => {
  const [currentFilter, setCurrentFilter] = React.useState('all')
  const onSearch = (event: any) => {
    console.log(event)
  }

  const onSelectFilter = (value: string) => {
    setCurrentFilter(value)
  }

  return (
    <StyledWrapper>
      <TopRow>
        <AssetsFilterDropdown
          options={AssetFilterOptions()}
          value={currentFilter}
          onSelectFilter={onSelectFilter}
        />
        <SearchBar
          placeholder="Search"
          autoFocus={true}
          action={onSearch}
        />
      </TopRow>
    </StyledWrapper>
  )
}

export default MarketView
