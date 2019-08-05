/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledDateTH,
  StyledAdTH,
  StyledCategoryTH,
  StyledAdContentDiv,
  StyledLogo,
  StyledLogoDiv,
  StyledAdInfoDiv,
  StyledAdBrand,
  StyledAdInfo,
  StyledAdStatDiv,
  StyledAdStat,
  StyledAdStatActions,
  StyledCategoryContentDiv,
  StyledCategoryName,
  StyledCategoryActions,
  StyledAdLink,
  StyledNoAdHistoryDiv,
  StyledNoLogoDiv,
  StyledTD
} from './style'
import { Row } from '../../../components/dataTables/table'
import { StyledTable } from '../../../components/dataTables/table/style'
import ThumbLikePicker from '../thumbLikePicker'
import CategoryLikePicker from '../categoryLikePicker'
import DropMenu from '../dropMenu'
import AdRowsDetails from '../adRowsDetails'
import { getLocale } from '../../../helpers'

export interface DetailRow {
  id: string
  date: string
  adDetailRows: AdDetailRow[]
}

export type AdAction = 'click' | 'dismiss' | 'view' | 'landed'

interface AdContent {
  brand: string
  brandInfo: string
  brandLogo: string
  brandUrl: string
  brandDisplayUrl: string
  likeAction: number
  adAction: AdAction
  savedAd: boolean
  flaggedAd: boolean
  logoUrl?: string
  onThumbUpPress?: () => void
  onThumbDownPress?: () => void
  onMenuSave?: () => void
  onMenuFlag?: () => void
}

interface CategoryContent {
  category: string
  optAction: number
  onOptInAction?: () => void
  onOptOutAction?: () => void
}

export interface AdDetailRow {
  id: string
  adContent: AdContent
  categoryContent: CategoryContent
}

export interface Props {
  header: string[]
  id?: string
  testId?: string
  children?: React.ReactNode
  headerColor?: boolean
  rows?: DetailRow[]
  allItems: number
}

export default class TableAdsHistory extends React.PureComponent<Props, {}> {
  getAdsHistoryTable = (header: string[], rows: DetailRow[]): any => {
    return (
      <StyledTable>
        {
          header ?
            <thead>
              <tr>
                {
                  this.getHeader(header)
                }
              </tr>
            </thead>
          : null
        }
        <tbody>
          {
            this.getRows(rows)
          }
        </tbody>
      </StyledTable>
    )
  }

  getHeader = (header: string[]) => {
    if (!header) {
      return
    }

    return (
      <>
        <StyledDateTH>{header[0]}</StyledDateTH>
        <StyledAdTH>{header[1]}</StyledAdTH>
        <StyledCategoryTH>{header[2]}</StyledCategoryTH>
      </>
    )
  }

  getRows = (rows: DetailRow[]) => {
    return rows.map((row: DetailRow, i: number) => {
      const detailRows = this.getDetailRows(row.adDetailRows)
      return (
        detailRows && detailRows.length > 0 ?
          <AdRowsDetails key={i} row={row} rowIndex={i} detailRows={detailRows} />
          : null
      )
    })
  }

  getCategoryContent = (content: CategoryContent) => {
    return (
      <StyledCategoryContentDiv>
        <StyledCategoryName>
          {content.category}
        </StyledCategoryName>
        <StyledCategoryActions>
          <CategoryLikePicker
            optAction={content.optAction}
            onOptIn={content.onOptInAction}
            onOptOut={content.onOptOutAction}
          />
        </StyledCategoryActions>
      </StyledCategoryContentDiv>
    )
  }

  getAdContent = (content: AdContent) => {
    return (
      <StyledAdContentDiv>
        <StyledAdLink href={content.brandUrl} target={'_blank'}>
          {
            content.logoUrl ?
              <StyledLogoDiv>
                <StyledLogo src={content.logoUrl} />
              </StyledLogoDiv>
              :
              <StyledNoLogoDiv />
          }
          <StyledAdInfoDiv>
            <StyledAdBrand>{content.brand}</StyledAdBrand>
            <StyledAdInfo>{content.brandInfo}</StyledAdInfo>
            <StyledAdInfo>{content.brandDisplayUrl}</StyledAdInfo>
          </StyledAdInfoDiv>
        </StyledAdLink>
        <StyledAdStatDiv>
          <StyledAdStat>
            {getLocale(content.adAction)}
          </StyledAdStat>
          <div />
          <StyledAdStatActions>
            <ThumbLikePicker
              likeAction={content.likeAction}
              onThumbUpPress={content.onThumbUpPress}
              onThumbDownPress={content.onThumbDownPress}
            />
            <DropMenu
              currentlySaved={content.savedAd}
              currentlyFlagged={content.flaggedAd}
              onMenuSave={content.onMenuSave}
              onMenuFlag={content.onMenuFlag}
            />
          </StyledAdStatActions>
        </StyledAdStatDiv>
      </StyledAdContentDiv>
    )
  }

  getDetailRows = (rows: AdDetailRow[]): Row[] | undefined => {
    if (!rows) {
      return
    }
    let filteredRows = rows
    if (this.props.allItems === 1) {
      filteredRows = rows.filter((row: AdDetailRow) => {
        return row.adContent && row.adContent.savedAd
      })
    } else if (this.props.allItems === 2) {
      filteredRows = rows.filter((row: AdDetailRow) => {
        return row.adContent && row.adContent.likeAction === 1
      })
    }
    return filteredRows.map((row: AdDetailRow, i: number): Row => {
      const cell: Row = {
        content: [
          {
            content: (
              <StyledTD key={i} />
            )
          },
          {
            content: (
              this.getAdContent(row.adContent)
            )
          },
          {
            content: (
              this.getCategoryContent(row.categoryContent)
            )
          }
        ]
      }
      return cell
    })
  }

  render () {
    const { id, testId, header, rows } = this.props
    return (
      <div id={id} data-test-id={testId} key={id}>
      {
        rows ?
          this.getAdsHistoryTable(header, rows)
          :
          <StyledNoAdHistoryDiv>
            {
              getLocale('noAdsHistory')
            }
          </StyledNoAdHistoryDiv>
      }
      </div>

    )
  }
}
