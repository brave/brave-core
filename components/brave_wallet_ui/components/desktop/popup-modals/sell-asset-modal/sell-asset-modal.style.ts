// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Assets
import WarningCircleFilled from '../../../../assets/svg-icons/warning-circle-filled.svg'

// Shared Styles
import { AssetIconProps, AssetIconFactory } from '../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  padding: 0 24px 24px 24px;
`

export const InputSection = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 27px 24px 24px 24px;
  border-radius: 12px;
  background-color: ${(p) => p.theme.color.background01};
  margin-bottom: 16px;
`

export const Text = styled.span<{
  textSize?: '32px' | '22px' | '20px' | '18px' | '16px' | '14px' | '12px'
  isBold?: boolean
  textColor?: 'text01' | 'text02' | 'text03'
  maintainHeight?: boolean
  textAlign?: 'left' | 'right'
  marginRight?: number
}>`
  --text01: ${(p) => p.theme.color.text01};
  --text02: ${(p) => p.theme.color.text02};
  --text03: ${(p) => p.theme.color.text03};
  --line-height: ${(p) => (p.textSize === '12px' ? '18px' : p.textSize === '14px' ? '24px' : p.textSize === '22px' ? '32px' : p.textSize === '32px' ? '48px' : '20px')}
  font-family: 'Poppins';
  color: ${(p) =>
    p.textColor ? `var(--${p.textColor})` : p.theme.color.text01};
  font-size: ${(p) => (p.textSize ? p.textSize : '18px')};
  font-weight: ${(p) => (p.isBold ? 500 : 400)};
  height: ${(p) => (p.maintainHeight ? 'var(--line-height)' : 'unset')};
  line-height: var(--line-height);
  letter-spacing: 0.02em;
  text-align: ${(p) => (p.textAlign ? p.textAlign : 'center')};
  word-wrap: wrap;
  margin-right: ${(p) => p.marginRight ? p.marginRight : 0}px;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '32px',
  height: 'auto'
})

export const AmountInput = styled.input`
  font-family: 'Poppins';
  color: ${(p) => p.theme.color.text01};
  font-weight: 500;
  font-size: 32px;
  line-height: 48px;
  text-align: left;
  width: 100%;
  outline: none;
  background-image: none;
  box-shadow: none;
  border: none;
  background: none;
  background-color: none;
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
  }
`

export const PresetButton = styled.button<{ marginRight?: number }>`
  /* #F4F6F8 does not exist in the design system */
  --button-background: #F4F6F8;
  /* rgba(218, 220, 232, 0.4) does not exist in the design system */
  --button-background-hover: rgba(218, 220, 232, 0.4);
  @media (prefers-color-scheme: dark) {
      /* rgb(18, 19, 22) does not exist in the design system */
    --button-background: rgb(18, 19, 22);
    --button-background-hover: ${(p) => p.theme.color.background01};
    }
  display: flex;
  font-family: 'Poppins';
  cursor: pointer;
  border: none;
  outline: none;
  background: none;
  background-color: var(--button-background);
  border-radius: 4px;
  font-size: 11px;
  font-weight: 600;
  line-height: 16px;
  color: ${(p) => p.theme.color.text02};
  margin-right: ${(p) => p.marginRight ? p.marginRight : 0}px;
  padding: 4px 8px;
  &:hover {
    background-color: var(--button-background-hover);
  }
`

export const ErrorBox = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  padding: 16px;
  border-radius: 8px;
  background: ${(p) => p.theme.color.errorBackground};
  margin-bottom: 16px;
`

export const ErrorIcon = styled.div`
  -webkit-mask-image: url(${WarningCircleFilled});
  height: 20px;
  width: 20px;
  mask-image: url(${WarningCircleFilled});
  mask-size: contain;
  margin-right: 18px;
  background: ${(p) => p.theme.color.errorIcon}
`
