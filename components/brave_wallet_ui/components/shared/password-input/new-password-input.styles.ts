// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// styles
import { ToggleVisibilityButton } from '../../shared/style'

// svgs
import CheckmarkSvg from '../../../assets/svg-icons/big-checkmark.svg'

interface StyleProps {
  hasError: boolean
  showPassword?: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
`

export const InputWrapper = styled.div`
  display: flex;
  position: relative;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;

  & > ${ToggleVisibilityButton} {
    position: absolute;
    right: 8px;
    z-index: 100;
  }
`

export const Input = styled.input<StyleProps>`
  box-sizing: border-box;
  width: 100%;
  outline: none;
  background-image: none;
  background-color: ${(p) => p.hasError ? p.theme.color.errorBackground : p.theme.color.background02};
  box-shadow: none;
  
  border: ${(p) => p.hasError
    ? `4px solid ${p.theme.color.errorBorder}`
    : `1px solid ${p.theme.color.interactive08}`
  };
  padding: ${(p) => p.hasError
    ? 7
    : 10
  }px;
  
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  margin: 0px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
  }
  :focus {
    outline: none;
  }
  ::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  ::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
`

export const InputLabel = styled.label`
  display: block;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text03};
`

export const PasswordMatchRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: right;
  width: 100%;
  padding: 0px 12px;
`

export const PasswordMatchText = styled.p`
  color: ${(p) => p.theme.color.interactive05};
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 20px;
  text-align: right;
  letter-spacing: 0.01em;
`

export const PasswordMatchCheckmark = styled.div`
  display: inline-block;
  width: 10px;
  height: 10px;
  margin-right: 4px;
  background-color: ${(p) => p.theme.color.interactive05};
  mask: url(${CheckmarkSvg}) no-repeat 50% 50%;
  mask-size: contain;
  vertical-align: middle;
`
