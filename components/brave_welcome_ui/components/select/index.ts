/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import palette from 'brave-ui/theme/colors'
import caratUrl from './carat.svg'

export interface SelectBoxProps {
  id?: string
  multiple?: boolean
  autoFocus?: boolean
  disabled?: boolean
  value?: string
  onChange?: (e: any) => void
  children: React.ReactNode
}

export const SelectBox = styled('select')<SelectBoxProps>`
  box-sizing: border-box;
  position: relative;
  -webkit-font-smoothing: antialiased;
  border-radius: 4px;
  display: flex;
  align-items: center;
  width: 100%;
  appearance: none;
  height: 32px;
  color: ${p => p.theme.color.text};
  font-size: 12px;
  font-weight: 500;
  line-height: 18px;
  font-family: ${p => p.theme.fontFamily.heading};
  border: 1px solid ${p => p.theme.color.inputBorder};
  background: url(${caratUrl}) 97% / 16px no-repeat transparent;
  /* avoid text overflow w/ carat */
  -webkit-padding-start: 10px;
  -webkit-padding-end: 32px;
  outline-width: 2px;
  outline-color: ${p => p.theme.color.brandBrave};
  cursor: pointer;
  margin: 0 16px;

  > option {
    /* see https://github.com/brave/brave-browser/issues/4213 for info */
    color: ${palette.black};
  }

  ${(p: SelectBoxProps) => p.disabled
    ? css`
      user-select: none;
      opacity: 0.6;
    ` : ''
  }
  ${(p: SelectBoxProps) => p.multiple
    ? css`
      padding: 6px;
      background: none;
    ` : ''
  }
`
