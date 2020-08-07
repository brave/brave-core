/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import { getLocale } from '../../../../common/locale'

// Components
import { StyledTitleTab } from '../widgetTitleTab'
import {
  Header,
  StyledTitle,
  StyledAddIcon
} from './style'
import AddCardIcon from './assets/add-icon'

interface Props {
  isAlone: boolean
  stackPosition: number
  onAddCard: () => void
}

class AddCard extends React.PureComponent<Props, {}> {

  render () {
    const { isAlone, onAddCard, stackPosition } = this.props

    return (
      <StyledTitleTab onClick={onAddCard} isAlone={isAlone} stackPosition={stackPosition}>
        <Header>
          <StyledTitle>
            <StyledAddIcon>
              <AddCardIcon />
            </StyledAddIcon>
            <>
              {getLocale('addCardWidgetTitle')}
            </>
          </StyledTitle>
        </Header>
      </StyledTitleTab>
    )
  }
}

export const AddCardWidget = createWidget(AddCard)
