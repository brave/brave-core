// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as leo from '@brave/nala/tokens/css/variables'
import styled from 'styled-components'
import Collapse from '@brave/nala/react/collapse'
import { WarningBoxTitleRow } from '../shared-panel-styles'

// Shared Styles
import { Column, Text, styledScrollbarMixin } from '../../shared/style'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${leo.color.page.background};
  padding: 0px 16px;
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 0px;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 54px;
  min-height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-bottom: 13px;
`

export const AccountNameText = styled(Text)`
  margin-bottom: 2px;
  max-width: 80%;
  word-break: break-word;
  text-align: center;
`

export const PanelTitle = styled(Text)`
  text-align: center;
  margin-bottom: 15px;
`

export const MessageBox = styled.div<{ height?: string; width?: string }>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  border: 1px solid ${leo.color.divider.subtle};
  box-sizing: border-box;
  border-radius: 4px;
  width: ${(p) => (p.width ? p.width : '255px')};
  height: ${(p) => (p.height ? p.height : '140px')};
  /* A flex item's default min-height is its content size, so newline runs
     grew the box and the panel clipped them with no scrollbar. */
  min-height: 0;
  max-height: ${(p) => (p.height ? p.height : '140px')};
  flex-shrink: 1;
  padding: 8px 14px;
  margin-bottom: 14px;
  overflow-x: hidden;
  overflow-y: auto;
  position: relative;

  /* Overlay scrollbars stay hidden until hover, so overflow looks clipped. */
  &::-webkit-scrollbar {
    appearance: none;
    -webkit-appearance: none;
    width: 7px;
  }

  &::-webkit-scrollbar-thumb {
    border-radius: 4px;
    background-color: rgba(0, 0, 0, 0.5);
    box-shadow: 0 0 1px rgba(255, 255, 255, 0.5);
  }

  &::-webkit-scrollbar-track {
    background-color: transparent;
    border-radius: 8px;
  }
`

export const SignMessageBoxFrame = styled.div<{
  height?: string
  width?: string
}>`
  position: relative;
  box-sizing: border-box;
  width: ${(p) => p.width ?? '255px'};
  height: ${(p) => p.height ?? '140px'};
  max-width: 100%;
  min-width: 0;
  margin-bottom: 14px;
  flex-shrink: 1;
`

export const FramedMessageBox = styled(MessageBox)`
  &&& {
    width: 100%;
    max-width: 100%;
    height: 100%;
    max-height: 100%;
    margin-bottom: 0;
  }
`

export const ScrollOverflowHint = styled.div`
  position: absolute;
  z-index: 1;
  left: 1px;
  right: 8px;
  bottom: 1px;
  height: 28px;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  pointer-events: none;
  border-radius: 0 0 3px 3px;
  /* Blank newline runs have nothing to fade, so the marker has to read
     on the empty page background. */
  background: linear-gradient(
    to bottom,
    transparent,
    ${leo.color.page.background} 55%
  );

  &::after {
    content: '';
    width: 7px;
    height: 7px;
    margin-bottom: 6px;
    border-right: 2px solid ${leo.color.icon.default};
    border-bottom: 2px solid ${leo.color.icon.default};
    transform: rotate(45deg);
  }
`

export const MessageHeader = styled(Text)`
  text-align: left;
  word-break: break-word;
  white-space: pre-wrap;
`

export const MessageHeaderSection = styled(MessageHeader)`
  margin: auto;
  text-align: center;
`

export const MessageText = styled(Text)`
  text-align: left;
  word-break: break-word;
  white-space: pre-wrap;
`

export const SignPanelButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
  gap: 8px;
`

export const WarningTitleRow = styled(WarningBoxTitleRow)`
  margin-bottom: 8px;
`

/**
 * The decoded call name and the fee are the two things the user has to read
 * before signing, so they get a card of their own rather than being buried in
 * the payload dump.
 */
export const CallSummaryBox = styled(Column).attrs({
  fullWidth: true,
  gap: '4px',
  padding: '12px 16px',
})`
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: ${leo.radius.m};
  background-color: ${leo.color.container.highlight};
  margin-bottom: 8px;
`

export const CallLabel = styled.span`
  font: ${leo.font.xSmall.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
  color: ${leo.color.text.tertiary};
  text-transform: uppercase;
`

/**
 * Monospaced because the content is a code identifier the user is being asked
 * to vet, not prose: a lookalike substitution in a call name should not blend
 * into a proportional font.
 */
export const CallText = styled.span`
  font: ${leo.font.monospace.default};
  color: ${leo.color.text.primary};
  text-align: center;
  word-break: break-all;
`

export const DetailsCollapse = styled(Collapse)`
  width: 100%;
  margin-bottom: 8px;

  --leo-collapse-summary-padding: 8px 12px;
  --leo-collapse-content-padding: 0px;
  --leo-collapse-radius: ${leo.radius.m};
  --leo-collapse-shadow: none;
  --leo-collapse-border-color: ${leo.color.divider.subtle};
  --leo-collapse-background-color: ${leo.color.container.background};
`

export const DetailsCollapseTitle = styled.span`
  font: ${leo.font.small.semibold};
  color: ${leo.color.text.secondary};
  text-align: start;
  vertical-align: middle;
`

/**
 * Bounded rather than free-growing: a nested call's `toHuman()` can run to
 * hundreds of lines, and the panel's Cancel/Sign row must stay on screen.
 */
export const PayloadJson = styled.pre`
  margin: 0;
  padding: 8px 12px;
  width: 100%;
  box-sizing: border-box;
  max-height: 180px;
  overflow: auto;
  font: ${leo.font.monospace.small};
  color: ${leo.color.text.secondary};
  text-align: left;
  white-space: pre-wrap;
  word-break: break-word;

  ${styledScrollbarMixin}
`

export const HeaderTitle = styled.div`
  font: ${leo.font.large.semibold};
  display: flex;
  align-items: center;
  text-align: center;
  color: ${leo.color.text.primary};
  margin: 4px 0 8px 0;
`
