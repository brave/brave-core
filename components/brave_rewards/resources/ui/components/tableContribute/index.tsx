/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledText,
  StyledRemove,
  StyledToggle,
  StyledTHOther,
  StyledTHLast,
  StyledToggleWrap
} from './style'
import Table, { Row } from '../../../components/dataTables/table/index'
import Profile, { Provider } from '../profile/index'
import { getLocale } from '../../../helpers'
import Tokens from '../tokens'

interface ProfileCell {
  verified: boolean
  name: string
  src: string
  provider?: Provider
}

export interface DetailRow {
  profile: ProfileCell
  attention: number
  onRemove?: () => void
  token?: { value: number, converted: number }
}

export interface Props {
  header: string[]
  showRowAmount?: boolean
  showRemove?: boolean
  id?: string
  children?: React.ReactNode
  headerColor?: boolean
  rows?: DetailRow[]
  numSites?: number
  allSites?: boolean
  onShowAll?: () => void
}

const removeIcon = require('./assets/close')

export default class TableContribute extends React.PureComponent<Props, {}> {
  getHeader = (header: string[]) => {
    if (!header) {
      return
    }

    if (this.props.showRemove) {
      header.push('')
    }

    let customStyle = {}

    if (this.props.headerColor) {
      customStyle = {
        border: 'none',
        'border-bottom': `1px solid #9F22A1`,
        padding: '0',
        color: '#9F22A1'
      }
    }

    return header.map((item: string, i: number) => {
      return {
        content: i === 0
        ? <div>{item}</div>
        : i === header.length - 1
          ? <StyledTHLast>{item}</StyledTHLast>
          : <StyledTHOther>{item}</StyledTHOther>,
        customStyle
      }
    })
  }

  getRows = (rows?: DetailRow[]): Row[] | undefined => {
    if (!rows) {
      return
    }

    return rows.map((row: DetailRow): Row => {
      const cell: Row = {
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
          {
            content: (
              <StyledText>
                {row.attention}%
              </StyledText>
            )
          }
        ]
      }

      if (row.token) {
        cell.content.push({
          content: (
            <Tokens
              value={row.token.value}
              converted={row.token.converted}
              size={'small'}
            />
          ),
          customStyle: {
            textAlign: 'right',
            paddingRight: '10px'
          }
        })
      }

      if (this.props.showRemove) {
        cell.content.push({
          content: (
            <StyledRemove onClick={row.onRemove}>{removeIcon}</StyledRemove>
          ),
          customStyle: {
            textAlign: 'right'
          }
        })
      }

      if (this.props.showRowAmount) {
        if (this.props.showRemove) {
          const remaining = (100 - row.attention) / 1.04
          const attention = row.attention / 1.04
          const diff = remaining + attention
          cell.customStyle = {
            background: `linear-gradient(
              to right,
              transparent 0%,
              transparent ${remaining}%,
              rgba(210, 198, 243, 0.39) ${remaining}%,
              rgba(210, 198, 243, 0.39) ${diff}%,
              transparent ${diff}%,
              transparent 100%
            )`
          }
        } else {
          const remaining = 100 - row.attention
          cell.customStyle = {
            background: `linear-gradient(90deg, transparent ${remaining}%, rgba(210, 198, 243, 0.39) ${row.attention}%)`
          }
        }
      }

      return cell
    })
  }

  render () {
    const { id, header, children, rows, allSites, onShowAll } = this.props
    const numSites = this.props.numSites || 0

    return (
      <div id={id}>
        <Table
          header={this.getHeader(header)}
          children={children}
          rows={this.getRows(rows)}
        />
        {
          !allSites && numSites > 0
          ? <StyledToggleWrap>
              <StyledToggle onClick={onShowAll}>{getLocale('seeAllSites', { numSites })}</StyledToggle>
            </StyledToggleWrap>
          : null
        }
      </div>
    )
  }
}
