// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'

interface StyledDialogWrapperProps {
  textDirection: string
}

export const StyledDialogWrapper = styled('div')<StyledDialogWrapperProps>`
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 5;
  display: flex;
  justify-content: center;
  align-items: center;
`

interface StyledDialogProps {
  textDirection: string
}

export const StyledDialog = styled('div')<StyledDialogProps>`
  position: relative;
  width: 437px;
  top: -50px;
  background-color: ${p => p.theme.color.contextMenuBackground};
  border-radius: 4px;
  padding: 32px 24px 24px 24px;
  display: flex;
  flex-direction: column;

  box-shadow: 0px 4px 12px 0px rgba(0, 0, 0, 0.2);
`

export const DialogTitle = styled('div')<{}>`
  font-weight: 600;
  font-size: 15px;
  line-height: 20px;
  color: #212529;

  @media (prefers-color-scheme: dark) {
    color: #F0F2FF;
  }
`

export const CloseButton = styled('button')`
  appearance: none;
  background: none;
  border: none;
  position: absolute;
  padding: 4px;
  box-sizing: content-box;
  margin: 0;
  right: 16px;
  top: 16px;
  width: 16px;
  height: 16px;
  cursor: pointer;
  border-radius: 100%;
  outline: none;
  transition: background .12s ease-in-out, box-shadow .12s ease-in-out;
  [dir=rtl] & {
    right: unset;
    left: 16px;
  }
  :hover, :focus-visible {
    background: rgba(255, 255, 255, .3);
  }
  :active {
    box-shadow: 0 0 0 4px rgba(255, 255, 255, .6);
  }

  color: #495057;
  @media (prefers-color-scheme: dark) {
    color: #C2C4CF;
  }
`

export const StyledInputLabel = styled('div')<{}>`
  margin-top: 18px;
  font-weight: 500;
  font-size: 13px;
  line-height: 16px;
  color: #495057;
  @media (prefers-color-scheme: dark) {
    color: #C2C4CF;
  }
`

export const StyledInput = styled('input')<{}>`
  outline: none;
  margin-top: 6px;
  width: 389px;
  height: 40px;
  padding: 10px 18px;
  border-radius: 4px;
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 400;
  line-height: 20px;
  letter-spacing: 0.01em;
  text-align: left;

  background: white;
  border: 1px solid #AEB1C2;
  color: #495057;
  @media (prefers-color-scheme: dark) {
    background: #1E2029;
    border: 1px solid #5E6175;
    color: #C2C4CF;
  }

  &:focus, :hover {
    border: 4px solid #A0A5EB;
    padding: 7px 15px;
  }

  &::placeholder {
    color: #84889C;
  }
`

export const StyledButtonsContainer = styled('div')<{}>`
  box-sizing: border-box;
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  justify-content: flex-end;
  margin-top: 24px;
  gap: 10px;
`

export const StyledButton = styled(Button as React.ComponentType<ButtonProps>)`
  &:focus {
    outline-offset: 2px;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 2px;
  }

  &:active {
    outline: none;
  }
`
