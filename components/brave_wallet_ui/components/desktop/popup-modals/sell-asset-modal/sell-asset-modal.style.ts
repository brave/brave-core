// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

// Assets
import WarningCircleFilled from '../../../../assets/svg-icons/warning-circle-filled.svg'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  Text as SharedText,
} from '../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  width: 100%;
  height: 100%;
  padding: 0 24px 24px 24px;
`

export const InputSection = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 27px 24px 24px 24px;
  border-radius: 12px;
  background-color: ${leo.color.page.background};
  margin-bottom: 16px;
`

export const Text = styled(SharedText)<{
  marginRight?: number
}>`
  letter-spacing: 0.02em;
  margin-right: ${(p) => (p.marginRight ? p.marginRight : 0)}px;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '32px',
  height: 'auto',
})

export const AmountInput = styled.input`
  color: ${leo.color.text.primary};
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
    color: ${leo.color.text.tertiary};
  }
`

export const PresetButton = styled.button<{ marginRight?: number }>`
  font: ${leo.font.xSmall.semibold};

  /* #F4F6F8 does not exist in the design system */
  --button-background: #f4f6f8;
  /* rgba(218, 220, 232, 0.4) does not exist in the design system */
  --button-background-hover: rgba(218, 220, 232, 0.4);
  @media (prefers-color-scheme: dark) {
    /* rgb(18, 19, 22) does not exist in the design system */
    --button-background: rgb(18, 19, 22);
    --button-background-hover: ${leo.color.page.background};
  }
  display: flex;
  cursor: pointer;
  border: none;
  outline: none;
  background: none;
  background-color: var(--button-background);
  border-radius: 4px;
  color: ${leo.color.text.secondary};
  margin-right: ${(p) => (p.marginRight ? p.marginRight : 0)}px;
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
  background: ${leo.color.systemfeedback.errorBackground};
  margin-bottom: 16px;
`

export const ErrorIcon = styled.div`
  -webkit-mask-image: url(${WarningCircleFilled});
  min-height: 20px;
  min-width: 20px;
  mask-image: url(${WarningCircleFilled});
  mask-size: contain;
  margin-right: 18px;
  background: ${leo.color.systemfeedback.errorIcon};
  mask-repeat: no-repeat;
`
