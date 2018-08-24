/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledBox, StyledTitle, StyledValue, StyledText } from './style'
import { getLocale } from '../../../helpers'
import Button from '../../../components/buttonsIndicators/button'

export interface Props {
  id?: string
  onClose: () => void
  amount: number
  date: string
}

export default class GrantComplete extends React.PureComponent<Props, {}> {
  render () {
    const { id, onClose, amount, date } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledBox>
          <StyledTitle>{getLocale('newTokenGrant')}</StyledTitle>
          <StyledValue>{amount} BAT</StyledValue>
          <StyledTitle>{getLocale('grantExpire')}</StyledTitle>
          <StyledValue>{date}</StyledValue>
        </StyledBox>
        <StyledText>{getLocale('grantDisclaimer')}</StyledText>
        <Button
          text={getLocale('ok')}
          size={'call-to-action'}
          type={'accent'}
          onClick={onClose}
        />
      </StyledWrapper>
    )
  }
}
