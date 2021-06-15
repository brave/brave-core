/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledRemoveAll
} from './style'
import Modal from 'brave-ui/components/popupModals/modal/index'
import TablePending, { DetailRow } from '../tablePending/index'
import { getLocale } from 'brave-ui/helpers'

export interface Props {
  rows: DetailRow[]
  onClose: () => void
  onRemoveAll?: () => void
  id?: string
}

export default class ModalPending extends React.PureComponent<Props, {}> {
  render () {
    const { id, onClose, rows, onRemoveAll } = this.props

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledTitle>
            {getLocale('pendingContributions')}
            {
              onRemoveAll
              ? <StyledRemoveAll onClick={onRemoveAll}>
                {getLocale('pendingContributionRemoveAll')}
              </StyledRemoveAll>
              : null
            }
          </StyledTitle>
          <TablePending
            id={'pendingContributionTable'}
            rows={rows}
            children={getLocale('pendingContributionEmpty')}
          />
        </StyledWrapper>
      </Modal>
    )
  }
}
