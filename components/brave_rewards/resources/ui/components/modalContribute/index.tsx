/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledTitle, StyledContent, StyledNum } from './style'
import Modal from '../../../components/popupModals/modal/index'
import TableContribute, { DetailRow } from '../tableContribute/index'
import { getLocale } from '../../../helpers'

export interface Props {
  rows: DetailRow[]
  onClose: () => void
  id?: string
}

export default class ModalContribute extends React.PureComponent<Props, {}> {
  get headers () {
    return [
      getLocale('site'),
      getLocale('rewardsContributeAttention')
    ]
  }

  render () {
    const { id, onClose, rows } = this.props
    const numSites = rows && rows.length || 0

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledTitle>{getLocale('rewardsContribute')}</StyledTitle>
          <StyledContent>
            {getLocale('rewardsContributeText1')} <StyledNum>{numSites}</StyledNum> {getLocale('sites')}.
          </StyledContent>
          <TableContribute
            header={this.headers}
            rows={rows}
            numSites={numSites}
            allSites={true}
            showRowAmount={true}
            showRemove={true}
          />
        </StyledWrapper>
      </Modal>
    )
  }
}
