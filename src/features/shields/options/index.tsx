/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledOptions, StyledOption, StyledOptionCheck, StyledOptionText } from './style'
import { CheckIcon } from '../../../components/icons'

export interface OptionProps {
  selected: boolean
  onClick: (event: React.MouseEvent<HTMLDivElement>) => void
  children: React.ReactNode
}

export class Option extends React.PureComponent<OptionProps, {}> {
  render () {
    const { selected, onClick, children } = this.props
    return (
      <StyledOption
        onClick={onClick}
        selected={selected}
      >
        <StyledOptionCheck>{selected ? <CheckIcon /> : null}</StyledOptionCheck>
        <StyledOptionText>{children}</StyledOptionText>
      </StyledOption>
    )
  }
}

export interface OptionsProps {
  visible: boolean
  position?: {
    top?: string
    right?: string
    bottom?: string
    left?: string
  }
  children: React.ReactNode
}

export class Options extends React.PureComponent<OptionsProps, {}> {
  render () {
    const { visible, position, children } = this.props
    return (
      <StyledOptions visible={visible} position={position}>{children}</StyledOptions>
    )
  }
}
