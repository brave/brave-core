// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

// types
import { PanelButtonTypes } from './index'

// images
import CloseIcon from '../../../../assets/svg-icons/close.svg'
import KeyIcon from '../../../../assets/svg-icons/key-icon.svg'
import CheckIcon from '../../assets/filled-checkmark.svg'

// styles
import { walletButtonFocusMixin } from '../../../shared/style'
import { Link } from 'react-router-dom'

interface StyledButtonProps {
  buttonType: PanelButtonTypes
  disabled?: boolean
  addTopMargin?: boolean
  to?: string // for links & routes
  maxHeight?: string
  minHeight?: string
  minWidth?: string
  isV2?: boolean
}

const StyledButtonCssMixin = (p: StyledButtonProps) => {
  return css<StyledButtonProps>`
    ${walletButtonFocusMixin}
    font-family: Poppins;
    font-style: normal;
    min-width: ${(p) => p?.minWidth || 'unset'};
    min-height: ${(p) => p?.minHeight || 'unset'};
    max-height: ${(p) => p?.maxHeight || 'unset'};
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: ${(p) => (p.disabled ? 'default' : 'pointer')};
    border-radius: 40px;
    padding: ${p.isV2 ? '18px 24px' : '10px 22px'};
    outline: none;
    margin-top: ${(p) => (p?.addTopMargin ? '8px' : '0px')};

    background-color: ${(p) =>
      p.disabled
        ? p.isV2
          ? leo.color.icon.disabled
          : p.theme.color.disabled
        : p.buttonType === 'primary' ||
          p.buttonType === 'confirm' ||
          p.buttonType === 'sign'
        ? p.isV2
          ? leo.color.primitive.primary[60]
          : p.theme.palette.blurple500
        : p.buttonType === 'danger'
        ? p.theme.color.errorBorder
        : 'transparent'};

    border: ${(p) =>
      p.buttonType === 'secondary' || p.buttonType === 'reject'
        ? `1px solid ${p.theme.color.interactive08}`
        : 'none'};

    pointer-events: ${(p) => (p.disabled ? 'none' : 'auto')};

    text-decoration: none;
  `
}

export const StyledButton = styled.button<StyledButtonProps>`
  ${(p) => StyledButtonCssMixin(p)}
`

export const StyledLink = styled(Link).withConfig<StyledButtonProps>({
  shouldForwardProp: (prop) => {
    // prevents reactDOM errors (Link does not support these props)
    return prop !== 'minWidth' && prop !== 'maxHeight' && prop !== 'buttonType'
  }
})`
  ${(p) => StyledButtonCssMixin(p)}
`

export const ButtonText = styled.span<{
  buttonType: PanelButtonTypes
  isV2?: boolean
}>`
  font-family: Poppins;
  font-size: ${(p) => (p.isV2 ? '16px' : '13px')};
  font-weight: 600;
  line-height: ${(p) => (p.isV2 ? '24px' : '20px')};
  color: ${(p) =>
    p.buttonType === 'secondary' ||
    p.buttonType === 'reject' ||
    p.buttonType === 'cancel'
      ? leo.color.text.secondary
      : p.theme.palette.white};
`

export const RejectIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${CloseIcon});
  mask-image: url(${CloseIcon});
  mask-size: 100%;
  margin-right: 10px;
`

export const SignIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${KeyIcon});
  mask-image: url(${KeyIcon});
  mask-size: 100%;
  margin-right: 10px;
`

export const ConfirmIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${CheckIcon});
  mask-image: url(${CheckIcon});
  mask-size: 100%;
  margin-right: 10px;
`

export const LaunchIcon = styled(Icon).attrs({ name: 'launch' })`
  --leo-icon-size: 20px;
  --leo-icon-color: ${leo.color.white};
  margin-left: 8px;
  margin-bottom: 1px;
`
