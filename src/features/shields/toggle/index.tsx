/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledSlider, StyledBullet, StyleToggle, StyledCheckbox } from './style'

export interface Props {
  testId?: string
  checked?: boolean
  disabled?: boolean
  onChange?: (e: any) => void
  id?: string
  readOnly?: boolean
  autoFocus?: boolean
  size?: 'large' | 'small'
  type?: 'dark' | 'light' | 'default'
}

export interface ToggleState {
  checked?: boolean
}

export class Toggle extends React.PureComponent<Props, ToggleState> {
  static defaultProps = {
    checked: false,
    size: 'small',
    type: 'default',
    disabled: false
  }

  constructor (props: Props) {
    super(props)
    this.state = { checked: props.checked }
    this.handleChange = this.handleChange.bind(this)
  }

  componentWillReceiveProps (nextProps: Props) {
    if ('checked' in nextProps) {
      this.setState({ checked: nextProps.checked })
    }
  }

  handleChange (e: any) {
    const { props } = this
    if (props.disabled) {
      return
    }
    if (!('checked' in props)) {
      this.setState({ checked: e.target.checked })
    }

    if (props.onChange) {
      props.onChange({ target: { checked: e.target.checked } })
    }
  }

  render () {
    const { id, testId, readOnly, disabled, autoFocus, size, type } = this.props
    const { checked } = this.state

    return (
      <StyledWrapper checked={checked} data-test-id={testId} size={size}>
        <StyleToggle size={size}>
          <StyledCheckbox
            type='checkbox'
            id={id}
            readOnly={readOnly}
            disabled={disabled}
            checked={checked}
            onChange={this.handleChange}
            autoFocus={autoFocus}
          />
          <StyledSlider htmlFor={id} checked={checked} size={size} disabled={disabled} />
          <StyledBullet htmlFor={id} checked={checked} size={size} disabled={disabled} type={type} />
        </StyleToggle>
      </StyledWrapper>
    )
  }
}
