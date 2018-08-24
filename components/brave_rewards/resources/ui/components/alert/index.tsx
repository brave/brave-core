/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledContent,
  StyledClose,
  StyledIcon,
  StyledError,
  StyledSuccess,
  StyledWarning
} from './style'
import { AlertCirlceIcon,
  CheckCircleIcon,
  CloseCircleIcon,
  CloseStrokeIcon } from '../../../components/icons'

export type Type = 'error' | 'success' | 'warning'

export interface Props {
  type: Type
  id?: string
  children?: React.ReactNode
  onClose?: () => void
  colored?: boolean
  bg?: boolean
}

export default class Alert extends React.PureComponent<Props, {}> {
  get icon () {
    switch (this.props.type) {
      case 'error':
        return (
          <StyledError>
            <CloseCircleIcon />
          </StyledError>
        )
      case 'success':
        return (
          <StyledSuccess>
            <CheckCircleIcon />
          </StyledSuccess>
        )
      case 'warning':
        return (
          <StyledWarning>
            <AlertCirlceIcon />
          </StyledWarning>
        )
    }

    return null
  }

  render () {
    const { id, children, onClose, colored, bg, type } = this.props

    return (
      <StyledWrapper id={id} type={type} bg={bg}>
        <StyledIcon>{this.icon}</StyledIcon>
        <StyledContent type={type} colored={colored} >
          {children}
        </StyledContent>
        {
          onClose
          ? <StyledClose>
            <CloseStrokeIcon />
          </StyledClose>
          : null
        }
      </StyledWrapper>
    )
  }
}
