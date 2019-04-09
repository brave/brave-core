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
  StyledToggleWrap,
  StyledLink
} from './style'
import Table, { Row } from '../../../components/dataTables/table'
import Profile, { Provider } from '../profile'
import { getLocale } from '../../../helpers'
import { Tokens, Tooltip } from '../'
import { CloseStrokeIcon, TrashOIcon } from '../../../components/icons'

interface ProfileCell {
  verified: boolean
  name: string
  src: string
  provider?: Provider
}

export interface DetailRow {
  profile: ProfileCell
  attention: number
  url: string
  onRemove?: () => void
  token?: { value: string, converted: string }
}

export interface Props {
  header: string[]
  showRowAmount?: boolean
  showRemove?: boolean
  id?: string
  testId?: string
  children?: React.ReactNode
  headerColor?: boolean
  rows?: DetailRow[]
  numSites?: number
  allSites?: boolean
  onShowAll?: () => void
  numExcludedSites?: number
  isMobile?: boolean
  isExcluded?: boolean
}

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
        padding: '8px 0',
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

    const { isExcluded } = this.props

    return rows.map((row: DetailRow): Row => {
      const cell: Row = {
        content: [
          {
            content: (
              <StyledLink href={row.url} target={'_blank'} data-test-id={'ac_link_' + row.profile.name}>
                <Profile
                  title={row.profile.name}
                  provider={row.profile.provider}
                  verified={row.profile.verified}
                  src={row.profile.src}
                  tableCell={this.props.isMobile}
                />
              </StyledLink>
            )
          }
        ]
      }

      if (!isExcluded) {
        cell.content.push({
          content: (
            <StyledText>
              {row.attention}%
            </StyledText>
          )
        })
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
        const actionTooltip = isExcluded
          ? getLocale('restoreSite')
          : getLocale('excludeSite')

        cell.content.push({
          content: (
            <Tooltip content={actionTooltip}>
              <StyledRemove onClick={row.onRemove}>
                {
                  isExcluded
                  ? <CloseStrokeIcon />
                  : <TrashOIcon />
                }
              </StyledRemove>
            </Tooltip>
          ),
          customStyle: {
            textAlign: 'right'
          }
        })
      }

      if (this.props.showRowAmount) {
        if (this.props.showRemove) {
          const remaining = (100 - row.attention) / 1.04
          cell.customStyle = {
            background: `linear-gradient(
              to right,
              transparent 0%,
              transparent ${remaining}%,
              rgba(210, 198, 243, 0.39) ${remaining}%,
              rgba(210, 198, 243, 0.39) 100%,
              transparent 100%,
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
    const { id, testId, header, children, rows, allSites, onShowAll } = this.props
    const numSites = this.props.numSites || 0
    const numExcludedSites = this.props.numExcludedSites || 0

    return (
      <div id={id} data-test-id={testId}>
        <Table
          header={this.getHeader(header)}
          children={children}
          rows={this.getRows(rows)}
        />
        {
          !allSites && (numSites > 0 || numExcludedSites > 0)
            ? <StyledToggleWrap>
              <StyledToggle onClick={onShowAll}>{getLocale('showAll')}</StyledToggle>
            </StyledToggleWrap>
            : null
        }
      </div>
    )
  }
}
