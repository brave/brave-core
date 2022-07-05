/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { CSSProperties } from 'styled-components'

import {
  StyledWrapper,
  StyledTH,
  StyledNoContent,
  StyledTable,
  StyledTHead,
  StyledTBody,
  StyledTD,
  ArrowDown,
  ArrowUp,
  ArrowWrapper
} from './style'

import { SortOrder } from '../../../constants/types'

export interface Header {
  id: string
  customStyle?: CSSProperties
  content: React.ReactNode
  sortable?: boolean
  sortOrder?: SortOrder
}
export interface Cell {
  customStyle?: CSSProperties
  content: React.ReactNode
}

export interface Row {
  id: string
  customStyle?: CSSProperties
  content: Cell[]
  data: any
  onClick?: (data: any) => void
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

export const Table = (props: Props) => {
  const { id, headers, rows, children, stickyHeaders, onSort } = props

  const onHeaderClick = React.useCallback((headerId: string, currentSortOrder: SortOrder) => {
    const newSortOrder = currentSortOrder === 'asc' ? 'desc' : 'asc'

    if (onSort) {
      onSort(headerId, newSortOrder)
    }
  }, [onSort])

  const onRowClick = (row: Row) => {
    if (row.onClick) {
      row.onClick(row.data)
    }
  }

  return (
    <StyledWrapper id={id}>
      {headers && headers.length > 0 &&
        <StyledTable>
          {headers &&
            <StyledTHead>
              <tr>
                {
                  headers.map((header) =>
                    <StyledTH
                      id={`${header.id}-th`}
                      key={`${header.id}-th`}
                      style={header.customStyle}
                      sortable={header.sortable}
                      sortOrder={header.sortOrder}
                      onClick={() => header.sortable && onHeaderClick(header.id, header.sortOrder ?? 'desc')}
                      stickyHeaders={stickyHeaders}
                    >
                      {header.sortable &&
                          <ArrowWrapper>
                            {header.sortOrder === 'asc' ? <ArrowUp /> : null}
                            {header.sortOrder === 'desc' ? <ArrowDown /> : null}
                          </ArrowWrapper>
                      }
                      {header.content}
                    </StyledTH>
                  )
                }
              </tr>
            </StyledTHead>
          }
          {rows &&
            <StyledTBody>
              {
                rows.map((row: Row, i: number) =>
                  <tr
                    id={row.id}
                    key={i}
                    style={row.customStyle}
                    onClick={() => onRowClick(row)}
                  >
                    {
                      row.content.map((cell: Cell, j: number) =>
                        <StyledTD
                          key={`${id}-td-${i}-${j}`}
                          style={cell.customStyle}
                        >
                          {cell.content}
                        </StyledTD>
                      )
                    }
                  </tr>
                )
              }
            </StyledTBody>
          }
        </StyledTable>
      }
      {(!rows || rows.length === 0) &&
        <StyledNoContent>
          {children}
        </StyledNoContent>
      }
    </StyledWrapper>
  )
}
