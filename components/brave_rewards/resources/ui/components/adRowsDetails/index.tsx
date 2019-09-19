/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledAdPortionTD,
  StyledAdsDateRow,
  StyledAdsDetailRow,
  StyledCaratIcon,
  StyledDateText,
  StyledHR,
  StyledHRDiv,
  StyledInnerStartTD,
  StyledSpaceDiv
  } from './style'
import {
  CaratTriangleDownSIcon, CaratTriangleRightSIcon
} from 'brave-ui/components/icons'
import { DetailRow } from '../tableAdsHistory'
import { Row, Cell } from 'brave-ui/components/dataTables/table'

export interface Props {
  id?: string
  row?: DetailRow
  rowIndex?: number
  detailRows?: Row[]
}

interface State {
  innerDetailVisible: boolean
}

export default class AdRowsDetails extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      innerDetailVisible: true
    }
  }

  setInnerVisible = () => {
    this.setState({
      innerDetailVisible: !this.state.innerDetailVisible
    })
  }

  render () {
    const { row, rowIndex, detailRows } = this.props
    return (
      <>
        <tr key={rowIndex}>
          <td colSpan={3}>
            <StyledAdsDetailRow>
              <StyledAdsDateRow onClick={this.setInnerVisible}>
                <StyledCaratIcon>
                {
                  this.state.innerDetailVisible
                  ? <CaratTriangleDownSIcon />
                  : <CaratTriangleRightSIcon />
                }
                </StyledCaratIcon>
                <StyledDateText>
                  {
                    row ? row.date : ''
                  }
                </StyledDateText>
                <StyledHRDiv>
                  <StyledHR />
                </StyledHRDiv>
              </StyledAdsDateRow>
            </StyledAdsDetailRow>
          </td>
        </tr>
        {
          detailRows && this.state.innerDetailVisible ?
          detailRows.map((detailRow: Row, j: number) => {
            return (
            <tr key={j}>
            {
              detailRow.content.map((detailCell: Cell, k: number) => {
                return k === 0 ?
                  <StyledInnerStartTD key={k}>
                    <StyledSpaceDiv />
                  </StyledInnerStartTD>
                :
                  <StyledAdPortionTD key={k}>
                    {detailCell.content}
                  </StyledAdPortionTD>
              })
            }
            </tr>
            )
          })
          : null
        }
      </>
    )
  }
}
