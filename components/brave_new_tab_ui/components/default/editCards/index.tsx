/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../../../common/locale'

// Components
import {
  StyledWrapper,
  StyledTitle,
  StyledEditIcon
} from './style'
import EditIcon from './assets/edit-icon.svg'

interface Props {
  onEditCards: () => void
}

export default class EditCards extends React.PureComponent<Props, {}> {
  render () {
    return (
      <StyledWrapper onClick={this.props.onEditCards}>
        <StyledTitle>
          <StyledEditIcon src={EditIcon} />
          <span>
            {getLocale('editCardsTitle')}
          </span>
        </StyledTitle>
      </StyledWrapper>
    )
  }
}
