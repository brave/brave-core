// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'
import {
  layoutSmallWidth, //
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const RecoveryPhraseContainer = styled.div<{ phraseLength: number }>`
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px 29px;
  flex-direction: row;
  flex-wrap: wrap;
  padding: 16px;
  width: 100%;
  height: 230px;
  overflow-y: ${(p) => (p.phraseLength > 12 ? 'auto' : 'hidden')};

  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    background-color: ${leo.color.neutral[20]};
    border-radius: 100px;
  }

  &::-webkit-scrollbar-track {
    background-color: transparent;
    border-radius: 8px;
  }

  @media screen and (max-width: ${layoutSmallWidth}px) {
    grid-template-columns: repeat(2, 1fr);
    overflow-y: auto;
  }
`

export const RecoveryBubble = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  padding: 8px;
  border-radius: 4px;
  border: 1px solid ${leo.color.divider.subtle};

  font: ${leo.font.small.semibold};
  color: ${leo.color.text.primary};
  background-color: ${leo.color.container.background};
`

export const WordPos = styled.span`
  color: ${leo.color.text.tertiary};
  margin-right: 9px;
`

export const RecoveryBubbleBadge = styled.p`
  font: ${leo.font.xSmall.regular};
  position: absolute;
  top: -15px;
  left: -8px;
  color: ${leo.color.white};
  background-color: ${leo.color.primitive.brands.primaryFixed};
  width: 40px;
  border-radius: 4px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
`

export const FrostedGlass = styled.div`
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  border-radius: 8px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  flex: 1;
  background: rgba(255, 255, 255, 0.4);
  backdrop-filter: blur(6.5px);
`

export const HiddenPhraseContainer = styled.div`
  font: ${leo.font.components.tableheader};
  position: relative;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  padding: 0;
  height: 100%;
  min-height: 100%;
  cursor: pointer;

  & p {
    text-align: center;
    margin-top: 10px;
    color: ${leo.color.page.background};
  }
`

export const EyeOffIcon = styled(Icon).attrs({
  name: 'eye-off',
})`
  --leo-icon-size: 24px;
  --leo-icon-color: ${leo.color.icon.default};
`
