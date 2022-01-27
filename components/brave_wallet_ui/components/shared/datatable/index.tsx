/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTH,
  StyledNoContent,
  StyledTable,
  StyledTHead,
  StyledTBody,
  StyledTD,
  StyledTR,
  ArrowDown,
  ArrowUp,
  ArrowWrapper
} from './style'
import { SortOrder } from '../../../constants/types'

export interface Header {
  id: string
  customStyle?: {[key: string]: string}
  content: React.ReactNode
  sortable?: boolean
  sortOrder?: SortOrder
}
export interface Cell {
  customStyle?: {[key: string]: string}
  content: React.ReactNode
}

export interface Row {
  customStyle?: {[key: string]: string}
  content: Cell[]
}

export interface Props {
  id?: string
  headers?: Header[]
  children?: React.ReactNode
  rows?: Row[]
  rowTheme?: {[key: string]: string}
  stickyHeaders?: boolean
  onSort?: (column: string, newSortOrder: SortOrder) => void
}

const Table = (props: Props) => {
  const { id, headers, rows, children, stickyHeaders, onSort } = props

  const onHeaderClick = (headerId: string, currentSortOrder: SortOrder) => {
    const newSortOrder = currentSortOrder === 'asc' ? 'desc' : 'asc'

    if (onSort) {
      onSort(headerId, newSortOrder)
    }
  }

  const renderHeaders = (headers: Header[]) => {
    return headers.map((header, index) => {
      const { id, content, sortable, sortOrder, customStyle } = header

      return (
        <StyledTH
          key={`${id}-th-${index}`}
          customStyle={customStyle}
          sortable={sortable}
          sortOrder={sortOrder}
          onClick={() => sortable && onHeaderClick(id, sortOrder ?? 'desc')}
          stickyHeaders={stickyHeaders}
        >
          {sortable &&
            <ArrowWrapper>
              {sortOrder === 'asc' ? <ArrowUp /> : null}
              {sortOrder === 'desc' ? <ArrowDown /> : null}
            </ArrowWrapper>
          }
          {content}
        </StyledTH>
      )
    })
  }

  return (
    <StyledWrapper id={id}>
      {
        headers && headers.length > 0
        ? <StyledTable>
          {
            headers
            ? <StyledTHead>
              <tr>
                {renderHeaders(headers)}
              </tr>
            </StyledTHead>
            : null
          }
          {
            rows
            ? <StyledTBody>
              {
                rows.map((row: Row, i: number) =>
                  <StyledTR
                    key={i}
                    customStyle={row.customStyle}
                  >
                    {
                      row.content.map((cell: Cell, j: number) =>
                        <StyledTD
                          key={`${id}-td-${i}-${j}`}
                          customStyle={cell.customStyle}
                        >
                          {cell.content}
                        </StyledTD>
                      )
                    }
                  </StyledTR>
                )
              }
            </StyledTBody>
            : null
          }
        </StyledTable>
        : null
      }
      {
        !rows || rows.length === 0
        ? <StyledNoContent>
          {children}
        </StyledNoContent>
        : null
      }
    </StyledWrapper>
  )
}

export default Table
