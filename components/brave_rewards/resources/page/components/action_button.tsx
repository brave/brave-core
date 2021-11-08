/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

interface Props {
  children: React.ReactNode
  onClick: () => void
}

export const StyledButton = styled.button`
  ${mixins.buttonReset}
  padding: 10px 22px;
  border: 1px solid var(--brave-palette-blurple500);
  background: var(--brave-palette-blurple500);
  color: var(--brave-palette-white);
  border-radius: 48px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  cursor: pointer;
  margin: 0 7px;

  &:hover, &:focus-visible {
    background: var(--brave-palette-blurple600);
  }

  &:active {
    background: var(--brave-palette-blurple700);
  }

  &.cancel {
    background: var(--brave-palette-white);
    color: var(--brave-palette-blurple500);

    &:hover, &:focus-visible {
      background: var(--brave-palette-blurple000);
    }

    &:active {
      background: var(--brave-palette-blurple200);
    }
  }
`

export function ActionButton (props: Props) {
  return (
    <StyledButton onClick={props.onClick}>
      {props.children}
    </StyledButton>
  )
}

export function CancelButton (props: Props) {
  return (
    <StyledButton className='cancel' onClick={props.onClick}>
      {props.children}
    </StyledButton>
  )
}
