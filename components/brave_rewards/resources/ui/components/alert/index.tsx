/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledContent, StyledClose, StyledIcon } from './style'

export type Type = 'error' | 'success' | 'warning'

export interface Props {
  type: Type
  id?: string
  children?: React.ReactNode
  onClose?: () => void
  colored?: boolean
  bg?: boolean
}

const success = require('./assets/success')
const error = require('./assets/error')
const close = require('./assets/close')
const warning = require('./assets/warning')

export default class Alert extends React.PureComponent<Props, {}> {
  get icon () {
    switch (this.props.type) {
      case 'error':
        return error
      case 'success':
        return success
      case 'warning':
        return warning
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
          ? <StyledClose>{close}</StyledClose>
          : null
        }
      </StyledWrapper>
    )
  }
}
