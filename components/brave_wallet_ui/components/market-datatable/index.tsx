import * as React from 'react'
import { MarketDataTableColumnTypes, SortOrder } from '../../constants/types'
import { StyledWrapper } from './style'
import Table, { Header, Row } from '../shared/datatable'

export interface MarketDataHeader extends Header {
  id: MarketDataTableColumnTypes
}

export interface Props {
  headers: MarketDataHeader[]
  rows: Row[]
  onSort?: (column: MarketDataTableColumnTypes, newSortOrder: SortOrder) => void
}

const MarketDataTable = (props: Props) => {
  const { headers, rows, onSort } = props

  return (
    <StyledWrapper>
      <Table
        headers={headers}
        rows={rows}
        onSort={onSort}
      />
    </StyledWrapper>
  )
}

export default MarketDataTable
