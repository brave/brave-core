/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledTH, StyledNoContent, StyledTable, StyledTBody, StyledTD, StyledTR, ArrowDown, ArrowUp } from './style'
import { SortOrder } from '../../../constants/types'

export interface Cell {
  customStyle?: {[key: string]: string}
  content: React.ReactNode
  sortOrder?: SortOrder
}

export interface Row {
  customStyle?: {[key: string]: string}
  content: Cell[]
}

export interface Props {
  id?: string
  header?: Cell[]
  children?: React.ReactNode
  rows?: Row[]
  rowTheme?: {[key: string]: string}
}

const Table = (props: Props) => {
  const { id, header, rows, children } = props

  return (
    <div id={id}>
      {
        header && header.length > 0
        ? <StyledTable>
          {
            header
            ? <thead>
              <tr>
              {
                header.map((cell: Cell, i: number) =>
                    <StyledTH
                      key={`${id}-th-${i}`}
                      customStyle={cell.customStyle}
                      sortOrder={cell.sortOrder}
                    >
                      {cell.sortOrder === 'ascending' && <ArrowUp />}
                      {cell.sortOrder === 'descending' && <ArrowDown />}
                      {cell.content}
                    </StyledTH>
                )
              }
              </tr>
            </thead>
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
    </div>
  )
}

export default Table
