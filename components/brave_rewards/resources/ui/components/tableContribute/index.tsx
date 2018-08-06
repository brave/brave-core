/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import {
  StyledWrapper,
  StyledText,
  StyledRemove,
  StyledToggle,
  StyledTHSite,
  StyledTHOther,
  StyledTHLast
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
  theme?: Theme
  rows?: DetailRow[]
  numSites?: number
  allSites?: boolean
  onShowAll?: () => void
}

interface Theme {
  headerColor?: CSS.Color
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

    let theme = {}

    if (this.props.theme && this.props.theme.headerColor) {
      theme = {
        border: 'none',
        'border-bottom': `1px solid ${this.props.theme.headerColor}`,
        padding: '0',
        color: this.props.theme.headerColor
      }
    }

    return header.map((item: string, i: number) => {
      return {
        content: i === 0
        ? <StyledTHSite>{item}</StyledTHSite>
        : i === header.length - 1
          ? <StyledTHLast>{item}</StyledTHLast>
          : <StyledTHOther>{item}</StyledTHOther>,
        theme: theme
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
                type={'small'}
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
              theme={{
                color: {
                  tokenNum: '#686978',
                  token: '#a7acb2',
                  text: '#a7acb2'
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
          theme: {
            textAlign: 'right'
          }
        })
      }

      if (this.props.showRowAmount) {
        if (this.props.showRemove) {
          const remaining = (100 - row.attention) / 1.04
          const attention = row.attention / 1.04
          const diff = remaining + attention
          cell.theme = {
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
          cell.theme = {
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
      <StyledWrapper id={id}>
        <Table
          header={this.getHeader(header)}
          children={children}
          rows={this.getRows(rows)}
        />
        {
          !allSites && numSites > 0
          ? <StyledToggle onClick={onShowAll}>{getLocale('seeAllSites', { numSites })}</StyledToggle>
          : null
        }
      </StyledWrapper>
    )
  }
}
