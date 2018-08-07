/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledTHLast,
  StyledType,
  StyledProvider
} from './style'
import Table, { Row } from '../../../components/dataTables/table/index'
import { getLocale } from '../../../helpers'
import Tokens from '../tokens'

export type TransactionType = 'deposit' | 'tipOnLike' | 'donation' | 'contribute' | 'recurringDonation'

type Description = string | { publisher: string, platform: string }

export interface DetailRow {
  date: string
  type: TransactionType
  description: Description
  amount: { value: number, converted: number, isNegative?: boolean }
}

export interface Props {
  id?: string
  children?: React.ReactNode
  rows?: DetailRow[]
}

export default class TableTransactions extends React.PureComponent<Props, {}> {
  private colors: Record<TransactionType, string> = {
    deposit: '#9f22a1',
    tipOnLike: '#696fdc',
    donation: '#696fdc',
    contribute: '#9752cb',
    recurringDonation: '#696fdc'
  }

  getHeader = () => {
    const header: string[] = [
      getLocale('date'),
      getLocale('type'),
      getLocale('description'),
      getLocale('amount')
    ]

    return header.map((item: string, i: number) => {
      return {
        content: i === header.length - 1
        ? <StyledTHLast>{item}</StyledTHLast>
        : item
      }
    })
  }

  getDescription = (desc: Description) => {
    if (typeof desc === 'string') {
      return desc
    }

    return (
      <>
        <span>{desc.publisher}</span> <StyledProvider>{getLocale('on')} {desc.platform}</StyledProvider>
      </>
    )
  }

  getRows = (rows?: DetailRow[]): Row[] | undefined => {
    if (!rows) {
      return
    }

    return rows.map((row: DetailRow): Row => {
      const cell: Row = {
        content: [
          {
            content: row.date
          },
          {
            content: <StyledType color={this.colors[row.type]}>{getLocale(row.type)}</StyledType>
          },
          {
            content: this.getDescription(row.description)
          },
          {
            content: (
              <Tokens
                value={row.amount.value}
                isNegative={row.amount.isNegative}
                converted={row.amount.converted}
                theme={{
                  color: {
                    tokenNum: this.colors[row.type] || '#686978',
                    token: '#686978',
                    text: '#9E9FAB'
                  },
                  size: {
                    text: '10px',
                    token: '12px',
                    tokenNum: '14px'
                  }
                }}
              />
            ),
            theme: {
              'text-align': 'right',
              'padding-right': '14px'
            }
          }
        ]
      }
      return cell
    })
  }

  render () {
    const { id, children, rows } = this.props

    return (
      <div id={id}>
        <Table
          header={this.getHeader()}
          children={children}
          rows={this.getRows(rows)}
        />
      </div>
    )
  }
}
