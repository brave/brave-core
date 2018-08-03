/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper } from './style'
import { getLocale } from '../../../helpers'
import ButtonPrimary from '../../../components/buttonsIndicators/buttonPrimary'
import ButtonGhost from '../../../components/buttonsIndicators/buttonGhost'

export interface Props {
  id?: string
  onAccept: () => void
  onLater: () => void
}

export default class GrantInit extends React.PureComponent<Props, {}> {
  render () {
    const { id, onAccept, onLater } = this.props

    return (
      <StyledWrapper id={id}>
        <div>
          <ButtonPrimary text={getLocale('accept')} size={'medium'} color={'brand'} onClick={onAccept}/>
        </div>
        <ButtonGhost text={getLocale('maybeLater')} size={'large'} color={'brand'} onClick={onLater}/>
      </StyledWrapper>
    )
  }
}
