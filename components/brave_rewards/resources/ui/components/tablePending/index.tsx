/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledRemove,
  StyledRemoveIcon,
  StyledLink
} from './style'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table/index'
import Profile, { Provider } from '../profile/index'
import Tokens from '../tokens/index'
import { getLocale } from 'brave-ui/helpers'
import { TrashOIcon } from 'brave-ui/components/icons'

interface ProfileCell {
  verified: boolean
  name: string
  src: string
  provider?: Provider
}

export type PendingType = 'tip' | 'ac' | 'recurring'

export interface DetailRow {
  profile: ProfileCell
  amount: {
    tokens: string
    converted: string
  }
  url: string
  type: PendingType
  date: React.ReactNode
  onRemove: () => void
}

export interface Props {
  id?: string
  children?: React.ReactNode
  rows?: DetailRow[]
}

export default class TableDonation extends React.PureComponent<Props, {}> {
  getRows (rows?: DetailRow[]): Row[] | undefined {
    if (!rows) {
      return
    }

    return rows.map((row: DetailRow): Row => {
      return {
        content: [
          {
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
          },
          {
            content: (
              <>{getLocale(`pendingType${row.type}`)}</>
            )
          },
          {
            content: (
              <>{row.date}</>
            )
          },
          {
            content: (
              <Tokens
                value={row.amount.tokens}
                converted={row.amount.converted}
                size={'small'}
              />
            ),
            customStyle: {
              'text-align': 'right',
              'padding': '0 7px 0 10px',
              'max-width': '130px'
            }
          },
          {
            content: (
              <StyledRemove onClick={row.onRemove}>
                <StyledRemoveIcon><TrashOIcon /></StyledRemoveIcon>
              </StyledRemove>
            )
          }
        ]
      }
    })
  }

  get headers (): Cell[] {
    const customStyle = {
      border: 'none',
      'border-bottom': `1px solid #696FDC`,
      padding: '0',
      color: '#696FDC'
    }

    return [
      {
        content: getLocale('site'),
        customStyle
      },
      {
        content: getLocale('type'),
        customStyle
      },
      {
        content: getLocale('pendingUntil'),
        customStyle
      },
      {
        content: getLocale('amount'),
        customStyle: {...customStyle,
          'text-align': 'right'
        }
      },
      {
        content: getLocale('remove'),
        customStyle: {...customStyle,
          'text-align': 'center',
          padding: '0 10px'
        }
      }
    ]
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
