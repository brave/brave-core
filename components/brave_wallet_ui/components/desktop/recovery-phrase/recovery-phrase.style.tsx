// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css'
import { layoutSmallWidth } from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const RecoveryPhraseContainer = styled.div<{ phraseLength: number }>`
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px 29px;
  flex-direction: row;
  flex-wrap: wrap;
  padding: 16px;
  width: 100%;
  height: 208px;
  overflow-y: ${p => p.phraseLength > 12 ? 'auto' : 'hidden'};

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

  @media screen and (max-width: ${layoutSmallWidth}px) {
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

export const RecoveryBubbleBadge = styled.p`
  position: absolute;
  top: -15px;
  left: -8px;
  color: ${(p) => p.theme.palette.white};
  background-color: ${(p) => p.theme.color.brandBat};
  width: 40px;
  border-radius: 4px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 10px;
  line-height: 15px;
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
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    text-align: center;
    margin-top: 10px;
    color: ${(p) => p.theme.color.background01};
  }
`

export const EyeOffIcon = styled(Icon).attrs({
  name: 'eye-off'
})`
  --leo-icon-size: 24px;
  --leo-icon-color: ${leo.color.icon.default};
`

export const Number = styled.span`
  color: ${leo.color.text.tertiary};
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 500;
  line-height: 16px;
`

export const IconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  --leo-button-radius: 50%;
  width: 24px;
  height: 60px;
  border: 1px solid ${leo.color.divider.interactive};
  border-radius: 16px;
`
