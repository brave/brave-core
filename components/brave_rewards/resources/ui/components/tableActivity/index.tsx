/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledLink
} from './style'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table/index'
import Profile, { Provider } from '../profile/index'
import Tokens from '../tokens/index'
import { getLocale } from 'brave-ui/helpers'

interface ProfileCell {
  verified: boolean
  name: string
  src: string
  provider?: Provider
}

export interface DetailRow {
  profile: ProfileCell
  amount: {
    tokens: string
    converted: string
  }
  url: string
  date?: number
}

export interface Props {
  id?: string
  children?: React.ReactNode
  rows?: DetailRow[]
  showDate?: boolean
}

export default class TableActivity extends React.PureComponent<Props, {}> {
  getRows (rows?: DetailRow[]): Row[] | undefined {
    if (!rows) {
      return
    }

    return rows.map((row: DetailRow): Row => {
      let content = []

      content.push({
        content: (
          <StyledLink href={row.url} target={'_blank'}>
            <Profile
              title={row.profile.name}
              provider={row.profile.provider}
              verified={row.profile.verified}
              src={row.profile.src}
            />
          </StyledLink>
        )
      })

      if (this.props.showDate) {
        content.push({
          content: (
            <>
              {row.date ? new Intl.DateTimeFormat('default', {
                month: 'short',
                day: 'numeric'
              }).format(row.date * 1000) : null}
            </>
          )
        })
      }

      content.push({
        content: (
          <Tokens
            value={row.amount.tokens}
            converted={row.amount.converted}
            size={'small'}
          />
        ),
        customStyle: {
          'text-align': 'right',
          'padding-right': '14px'
        }
      })

      return {
        content
      }
    })
  }

  get headers (): Cell[] {
    let cells = []
    cells.push({
      content: getLocale('site')
    })

    if (this.props.showDate) {
      cells.push({
        content: getLocale('date')
      })
    }

    cells.push({
      content: getLocale('amount'),
      customStyle: {
        'text-align': 'right',
        'padding-right': '14px'
      }
    })

    return cells
  }

  render () {
    const { id, children, rows } = this.props
    return (
      <div id={id}>
        <Table
          children={children}
          rows={this.getRows(rows)}
          header={this.headers}
        />
      </div>
    )
  }
}
