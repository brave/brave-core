/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledAdsHeaderWrapper,
  StyledAdsHistoryTitle,
  StyledAdsInfoText,
  StyledAdsInfoTextWrapper,
  StyledAdsPerHourText,
  StyledAdsSaveFiltered,
  StyledLink,
  StyledNoActivity,
  StyledNotSelectedLink,
  StyledSeparatorText,
  StyledSubTitleText,
  StyledText,
  StyledThumbDownFilter,
  StyledThumbDownNotSelectedFilter,
  StyledWrapper
} from './style'
import Modal from 'brave-ui/components/popupModals/modal/index'
import { getLocale } from 'brave-ui/helpers'
import TableAdsHistory, { DetailRow } from '../tableAdsHistory/index'
import { ThumbsupSIcon } from 'brave-ui/components/icons'

export interface Props {
  rows?: DetailRow[]
  onClose?: () => void
  id?: string
  adsPerHour?: number
  totalDays?: number
  hasSavedEntries?: boolean
  onSavedFilterClick?: () => void
  onAllFilterClick?: () => void
  onThumbFilterClick?: () => void
}

interface State {
  filterStatus: number
}

export default class ModalShowAdsHistory extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      filterStatus: 0
    }
  }

  get headers () {
    return [
      getLocale('date'),
      getLocale('ads'),
      getLocale('category')
    ]
  }

  onSavedFilterClick = () => {
    this.setState({
      filterStatus: 1
    })
    const savedFilterFn = this.props.onSavedFilterClick
    if (savedFilterFn) {
      savedFilterFn()
    }
  }

  onAllFilterClick = () => {
    this.setState({
      filterStatus: 0
    })
    const allFilterFn = this.props.onAllFilterClick
    if (allFilterFn) {
      allFilterFn()
    }
  }

  onThumbFilterClick = () => {
    this.setState({
      filterStatus: 2
    })
    const thumbFilterFn = this.props.onThumbFilterClick
    if (thumbFilterFn) {
      thumbFilterFn()
    }
  }

  render () {
    const { id, onClose, adsPerHour, hasSavedEntries, rows, totalDays } = this.props

    return (
      <Modal id={id} onClose={onClose} testId={'show-ads-history-modal'}>
        <StyledWrapper>
          <StyledAdsHistoryTitle>
            {getLocale('adsHistoryTitle')}
          </StyledAdsHistoryTitle>
          <StyledSubTitleText>
            {getLocale('adsHistorySubTitle', { totalDays: totalDays || '0' })}
          </StyledSubTitleText>
          <StyledAdsHeaderWrapper>
            <StyledAdsInfoTextWrapper>
              <StyledAdsInfoText>
                {getLocale('adsCurrentlyViewing')}
              </StyledAdsInfoText>
              <StyledAdsPerHourText>
                {adsPerHour || '0'}
              </StyledAdsPerHourText>
            </StyledAdsInfoTextWrapper>
              {
                rows && hasSavedEntries ?
                  <StyledAdsSaveFiltered>
                    <StyledText>
                      {
                        this.state.filterStatus !== 0 ?
                          <StyledLink onClick={this.onAllFilterClick}>
                            {
                              getLocale('all')
                            }
                          </StyledLink> :
                          <StyledNotSelectedLink>
                            {
                              getLocale('all')
                            }
                          </StyledNotSelectedLink>
                      }
                    </StyledText>
                    <StyledSeparatorText>|</StyledSeparatorText>
                    <StyledText>
                      {
                        this.state.filterStatus !== 1 ?
                          <StyledLink onClick={this.onSavedFilterClick}>
                            {
                              getLocale('saved')
                            }
                          </StyledLink> :
                          <StyledNotSelectedLink>
                            {
                              getLocale('saved')
                            }
                          </StyledNotSelectedLink>
                      }
                    </StyledText>
                    <StyledSeparatorText>|</StyledSeparatorText>
                    <StyledText>
                      {
                        this.state.filterStatus !== 2 ?
                          <StyledThumbDownFilter onClick={this.onThumbFilterClick}>
                            <ThumbsupSIcon />
                          </StyledThumbDownFilter> :
                          <StyledThumbDownNotSelectedFilter>
                            <ThumbsupSIcon />
                          </StyledThumbDownNotSelectedFilter>
                      }
                    </StyledText>
                  </StyledAdsSaveFiltered>
                : null
              }
          </StyledAdsHeaderWrapper>
          <TableAdsHistory
            rows={rows}
            allItems={this.state.filterStatus}
            header={this.headers}
          />
          {
            !rows || rows.length === 0 ?
              <StyledNoActivity>
                {getLocale('adsHistoryNone').replace('$1', String(totalDays || 0))}
              </StyledNoActivity>
            : null
          }
        </StyledWrapper>
      </Modal>
    )
  }
}
