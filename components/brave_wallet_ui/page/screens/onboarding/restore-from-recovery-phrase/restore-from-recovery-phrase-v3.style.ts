// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
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
  height: 195px;
  padding: 0 2px;
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

  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 16px;

  color: ${leo.color.text.primary};
  background-color: ${leo.color.container.background};
`

export const AlertWrapper = styled.div`
  leo-button {
    display: flex;
    height: 20px;
    flex-grow: 0;
    align-items: center;
  }
`
export const ErrorAlert = styled(Alert).attrs({
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
