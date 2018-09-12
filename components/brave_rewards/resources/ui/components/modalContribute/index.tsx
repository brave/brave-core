/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledContent,
  StyledNum,
  StyledRestore,
  StyledExcludedText
} from './style'
import Modal from '../../../components/popupModals/modal/index'
import TableContribute, { DetailRow } from '../tableContribute/index'
import { getLocale } from '../../../helpers'

export interface Props {
  rows: DetailRow[]
  onClose: () => void
  onRestore: () => void
  id?: string
  sortByAttentionDesc?: boolean
  numExcludedSites?: number
}

export default class ModalContribute extends React.PureComponent<Props, {}> {
  static defaultProps = {
    sortByAttentionDesc: true
  }

  get headers () {
    return [
      getLocale('site'),
      getLocale('rewardsContributeAttention')
    ]
  }

  getRestoreText () {
    return `(${getLocale('restoreAll')})`
  }

  getExclusionText (numSites: number) {
    return `${getLocale('excludedSitesText')} ${numSites}`
  }

  render () {
    const { id, onClose, onRestore, sortByAttentionDesc, rows, numExcludedSites } = this.props
    const numSites = rows && rows.length || 0

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledTitle>{getLocale('rewardsContribute')}</StyledTitle>
          <StyledContent>
            {getLocale('rewardsContributeText1')} <StyledNum>{numSites}</StyledNum> {getLocale('sites')}.
          </StyledContent>
          {
            numExcludedSites && numExcludedSites > 0
            ? <StyledExcludedText>
                {this.getExclusionText(numExcludedSites)}
                <StyledRestore onClick={onRestore}>
                  {this.getRestoreText()}
                </StyledRestore>
              </StyledExcludedText>
            : null
          }
          <TableContribute
            header={this.headers}
            rows={rows}
            numSites={numSites}
            allSites={true}
            showRowAmount={true}
            showRemove={true}
            sortByAttentionDesc={sortByAttentionDesc}
          />
        </StyledWrapper>
      </Modal>
    )
  }
}
