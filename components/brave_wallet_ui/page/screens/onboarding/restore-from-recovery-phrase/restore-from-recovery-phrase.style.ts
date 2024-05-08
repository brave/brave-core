// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Alert from '@brave/leo/react/alert'

import { layoutPanelWidth } from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { Text } from '../../../../components/shared/style'

export const RecoveryPhraseContainer = styled.div<{ phraseLength: number }>`
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  flex-direction: row;
  flex-wrap: wrap;
  width: 100%;
  height: 230px;
  padding: 2px;
  overflow-y: ${(p) => (p.phraseLength > 12 ? 'auto' : 'hidden')};

  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    background-color: ${leo.color.gray[20]};
    border-radius: 100px;
  }

  &::-webkit-scrollbar-track {
    background-color: transparent;
    border-radius: 8px;
  }

  @media screen and (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: repeat(2, 1fr);
  }
`

export const RecoveryBubble = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  padding: 8px;
  border-radius: 4px;
  border: 1px solid ${leo.color.divider.subtle};

  font: ${leo.font.default.semibold};
  font-weight: 600;

  color: ${leo.color.text.primary};
  background-color: ${leo.color.container.background};
`
export const RecoveryBaseCss = css`
  box-sizing: border-box;
  width: 100%;
  border: none;
  border-radius: 4px;
  text-align: left;
  vertical-align: middle;
  line-height: 40px;
  word-break: break-word;
  padding-left: 16px;
  padding-right: 16px;
  padding-top: 8px;
  padding-bottom: 8px;

  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  letter-spacing: 0.01em;

  color: ${(p) => p.theme.color.text01};
  background-color: ${(p) => p.theme.color.background02};
`

export const RecoveryTextArea = styled.textarea`
  ${RecoveryBaseCss}
  padding: 16px;
  height: 166px;
`

export const RecoveryTextInput = styled.input`
  ${RecoveryBaseCss}
  font-weight: 800;
`

export const InfoAlert = styled(Alert).attrs({
  kind: 'error',
  mode: 'simple'
})`
  --leo-alert-center-position: 'center';
  --leo-alert-center-width: '100%';
  width: 100%;
`

export const InputLabel = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.primary};
`

export const CheckboxText = styled.span`
  font: ${leo.font.default.semibold};
  color: ${leo.color.text.interactive};
`
