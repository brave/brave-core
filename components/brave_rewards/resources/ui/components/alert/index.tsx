/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import { StyledWrapper, StyledContent, StyledClose, StyledIcon } from './style'

export interface Theme {
  position?: CSS.PositionProperty
  top?: CSS.TopProperty<1>
  left?: CSS.LeftProperty<1>
}

export interface Props {
  type: 'error' | 'success' | 'warning'
  id?: string
  children?: React.ReactNode
  onClose?: () => void
  theme?: Theme
  color?: boolean
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

  get bgColor () {
    if (this.props.bg) {
      switch (this.props.type) {
        case 'error':
          return '#FFEEF1'
        case 'success':
          return '#E7F6FF'
        case 'warning':
          return '#FAF2DE'
      }
    }

    return '#fff'
  }

  get color () {
    if (this.props.color) {
      switch (this.props.type) {
        case 'error':
          return '#F36980'
        case 'success':
          return '#67D79D'
        case 'warning':
          return '#FF7900'
      }
    }

    return null
  }

  render () {
    const { id, children, onClose, theme } = this.props

    return (
      <StyledWrapper id={id} theme={theme} bgColor={this.bgColor}>
        <StyledIcon>{this.icon}</StyledIcon>
        <StyledContent color={this.color}>
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
