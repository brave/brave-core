// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Alert from '@brave/leo/react/alert'

import { LinkText, WalletButton } from '../../components/shared/style'

export const SnapBox = styled.div<{
  direction?: 'row' | 'column'
  alignment?: 'flex-start' | 'center' | 'flex-end'
}>`
  display: flex;
  flex-direction: ${(p) => p.direction ?? 'column'};
  align-items: ${(p) => p.alignment ?? 'flex-start'};
  gap: 8px;
`

export const SnapSection = styled.div`
  padding: 12px;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: ${leo.radius.m};
  background-color: ${leo.color.container.highlight};
`

export const SnapContainer = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`

export const SnapFooter = styled.div`
  display: flex;
  gap: 8px;
  padding-top: 12px;
  border-top: 1px solid ${leo.color.divider.subtle};
  justify-content: flex-end;
`

export const SnapHeading = styled.h2<{ size?: 'lg' | 'md' | 'sm' }>`
  margin: 0 0 4px;
  color: ${leo.color.text.primary};
  font: ${(p) =>
    p.size === 'lg'
      ? leo.font.large.semibold
      : p.size === 'sm'
        ? leo.font.small.semibold
        : leo.font.default.semibold};
`

export const SnapText = styled.p`
  margin: 0;
  font: ${leo.font.small.regular};
  line-height: 1.4;
  color: ${leo.color.text.primary};
`

const snapButtonStyles = css<{ isDestructive?: boolean }>`
  font: ${leo.font.small.semibold};
  padding: 8px 16px;
  border-radius: ${leo.radius.m};
  cursor: pointer;
  color: ${leo.color.white};
  border: 1px solid
    ${(p) =>
      p.isDestructive
        ? leo.color.systemfeedback.errorText
        : leo.color.button.background};
  background-color: ${(p) =>
    p.isDestructive
      ? leo.color.systemfeedback.errorText
      : leo.color.button.background};

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
`

export const SnapButton = styled(WalletButton)<{ isDestructive?: boolean }>`
  ${snapButtonStyles}
`

export const SnapRow = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 4px 0;
`

export const SnapRowLabel = styled.span`
  font: ${leo.font.small.semibold};
  color: ${leo.color.text.secondary};
`

export const SnapAddress = styled.span`
  font-family: monospace;
  font-size: 12px;
  line-height: 1.4;
  color: ${leo.color.text.secondary};
  word-break: break-all;
`

export const SnapCopyable = styled.span`
  font-family: monospace;
  font-size: 12px;
  line-height: 1.4;
  background-color: ${leo.color.container.highlight};
  color: ${leo.color.text.primary};
  padding: 6px 8px;
  border-radius: ${leo.radius.s};
  word-break: break-all;
  cursor: pointer;
`

export const SnapDivider = styled.hr`
  border: none;
  border-top: 1px solid ${leo.color.divider.subtle};
  margin: 8px 0;
`

export const SnapLink = styled(LinkText)`
  font: ${leo.font.small.regular};
`

export const SnapImage = styled.img`
  max-width: 100%;
  border-radius: ${leo.radius.s};
`

export const SnapInput = styled.input`
  box-sizing: border-box;
  width: 100%;
  padding: 6px 10px;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.primary};
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.neutral[30]};
  border-radius: ${leo.radius.m};
  outline: none;

  &::placeholder {
    color: ${leo.color.text.tertiary};
  }

  &:focus {
    border-color: ${leo.color.button.background};
  }
`

export const SnapDropdown = styled.select`
  box-sizing: border-box;
  width: 100%;
  padding: 6px 10px;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.primary};
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.neutral[30]};
  border-radius: ${leo.radius.m};
  outline: none;
  cursor: pointer;

  &:focus {
    border-color: ${leo.color.button.background};
  }

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
`

export const SnapField = styled.div`
  margin-bottom: 8px;
`

export const SnapFieldLabel = styled.label`
  display: block;
  margin-bottom: 4px;
  font: ${leo.font.small.semibold};
  color: ${leo.color.text.primary};
`

export const SnapFieldError = styled.span`
  display: block;
  margin-top: 4px;
  font: ${leo.font.xSmall.regular};
  color: ${leo.color.systemfeedback.errorText};
`

export const SnapSpinnerWrapper = styled.div`
  display: inline-flex;
  align-items: center;
  justify-content: center;
`

export const SnapBanner = styled(Alert)`
  width: 100%;
`

export const SnapUnknown = styled.div`
  font: ${leo.font.xSmall.regular};
  color: ${leo.color.text.tertiary};
`

export const SnapFallbackPre = styled.pre`
  font: ${leo.font.xSmall.regular};
  color: ${leo.color.text.secondary};
  margin: 0;
  white-space: pre-wrap;
  word-break: break-word;
`

export const SnapNoDataText = styled.p`
  margin: 0;
  font: ${leo.font.small.regular};
  color: ${leo.color.text.tertiary};
`
