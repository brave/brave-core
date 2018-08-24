/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledType,
  StyledDate,
  StyledRemove,
  StyledRemoveIcon,
  StyledToggle,
  StyledRecurringIcon,
  StyledToggleWrap
} from './style'
import Table, { Cell, Row } from '../../../components/dataTables/table/index'
import Profile, { Provider } from '../profile/index'
import Tokens from '../tokens/index'
import { getLocale } from '../../../helpers'

interface ProfileCell {
  verified: boolean
  name: string
  src: string
  provider?: Provider
}

type DonationType = 'donation' | 'tip' | 'recurring'

export interface DetailRow {
  profile: ProfileCell
  contribute: {
    tokens: number
    converted: number
  }
  type: DonationType
  text?: string | React.ReactNode
  onRemove?: () => void
}

export interface Props {
  id?: string
  children?: React.ReactNode
  rows?: DetailRow[]
  numItems?: number
  allItems?: boolean
  onShowAll?: () => void
  headerColor?: boolean
}

const removeIcon = require('./assets/close')
const monthlyIcon = require('./assets/monthly')

export default class TableDonation extends React.PureComponent<Props, {}> {
  getTypeContent (row: DetailRow): Cell {
    switch (row.type) {
      case 'recurring':
        return {
          content: (
            <>
              <StyledType>{getLocale('recurring')} <StyledRecurringIcon>{monthlyIcon}</StyledRecurringIcon></StyledType>
              <StyledRemove onClick={row.onRemove}>
                <StyledRemoveIcon> {removeIcon} </StyledRemoveIcon>{getLocale('remove')}
              </StyledRemove>
            </>
          )
        }
      case 'donation':
        return {
          content: (
            <>
              <StyledType>{getLocale('oneTime')}</StyledType>
              <StyledDate>{row.text}</StyledDate>
            </>
          )
        }
      case 'tip':
        return {
          content: (
            <>
              <StyledType>{getLocale('tipOnLike')}</StyledType>
              <StyledDate>{row.text}</StyledDate>
            </>
          )
        }
    }
  }

  getRows (rows?: DetailRow[]): Row[] | undefined {
    if (!rows) {
      return
    }

    return rows.map((row: DetailRow): Row => {
      return {
        content: [
          {
            content: (
              <Profile
                title={row.profile.name}
                provider={row.profile.provider}
                verified={row.profile.verified}
                src={row.profile.src}
              />
            )
          },
          this.getTypeContent(row),
          {
            content: (
              <Tokens
                value={row.contribute.tokens}
                converted={row.contribute.converted}
                size={'small'}
              />
            ),
            customStyle: {
              'text-align': 'right',
              'padding-right': '7px',
              'max-width': '40px'
            }
          }
        ]
      }
    })
  }

  get headers (): Cell[] {
    let customStyle = {}

    if (this.props.headerColor) {
      customStyle = {
        border: 'none',
        'border-bottom': `1px solid #696FDC`,
        padding: '0',
        color: '#696FDC'
      }
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
        content: getLocale('tokens'),
        customStyle: Object.assign({
          'text-align': 'right',
          'padding-right': '7px'
        }, customStyle)
      }
    ]
  }

  render () {
    const { id, children, rows, allItems, onShowAll } = this.props
    const numItems = this.props.numItems || 0

    return (
      <div id={id}>
        <Table
          children={children}
          rows={this.getRows(rows)}
          header={this.headers}
        />
        {
          !allItems && numItems > 0
          ? <StyledToggleWrap>
            <StyledToggle onClick={onShowAll}>
                {getLocale('seeAllItems', { numItems })}
            </StyledToggle>
          </StyledToggleWrap>
          : null
        }
      </div>
    )
  }
}
