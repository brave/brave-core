/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle
} from './style'
import Modal from '../../../components/popupModals/modal/index'
import TableDonation, { DetailRow } from '../tableDonation/index'
import { getLocale } from '../../../helpers'

export interface Props {
  rows: DetailRow[]
  onClose: () => void
  id?: string
}

export default class ModalDonation extends React.PureComponent<Props, {}> {
  render () {
    const { id, onClose, rows } = this.props
    const numRows = rows && rows.length || 0

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledTitle>{getLocale('donationTips')}</StyledTitle>
          <TableDonation
            rows={rows}
            allItems={true}
            numItems={numRows}
            headerColor={true}
          />
        </StyledWrapper>
      </Modal>
    )
  }
}
