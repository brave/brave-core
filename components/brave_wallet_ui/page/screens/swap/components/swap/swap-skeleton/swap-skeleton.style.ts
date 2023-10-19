// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import { Props } from './skeleton'
import { StyledDiv } from '../../shared-swap.styles'

export const Wrapper = styled.div<Props>`
  --background-color-primary: rgba(255, 255, 255, 0.6);
  --background-color-secondary: ${leo.color.purple[10]};
  @media (prefers-color-scheme: dark) {
    --background-color-primary: ${(p) => p.theme.color.background01};
    --background-color-secondary: ${(p) => p.theme.color.background02};
  }
  display: block;
  box-sizing: content-box;
  width: ${(p) => (p.width ? `${p.width}px` : '100%')};
  height: ${(p) => (p.height ? `${p.height}px` : '100%')};
  border-radius: ${(p) => (p.borderRadius ? `${p.borderRadius}px` : 'none')};
  background-color: var(
    --background-color--${(p) => (p.background ? p.background : 'primary')}
  );
  position: sticky;
  overflow: hidden;
`

export const SkeletonBox = styled.div<{ width?: number }>`
  display: block;
  height: 100%;
  transform: translate(400%);
  --start-distance: ${(p) =>
    p.width ? (p.width < 60 ? '-1000%' : '-300%') : '-300%'};
  --end-distance: ${(p) =>
    p.width ? (p.width < 60 ? '1000%' : '300%') : '300%'};
  @keyframes identifier {
    0% {
      transform: translate(var(--start-distance));
    }
    100% {
      transform: translate(0%);
    }
    100% {
      transform: translate(var(--end-distance));
    }
  }
  animation: 3s identifier infinite linear;
`

export const SkeletonIndicator = styled.div`
  width: 1px;
  height: 100%;
  background-color: ${(p) => p.theme.color.divider01};
  box-shadow: 0 0 100px 100px ${(p) => p.theme.color.divider01};
`

export const SwapSkeletonWrapper = styled(StyledDiv)`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  padding: 100px 0px;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  overflow-y: scroll;
  position: absolute;
  background-color: ${(p) => p.theme.color.background01};
  @media (prefers-color-scheme: dark) {
    background-color: ${(p) => p.theme.color.background02};
    padding: 80px 0px;
  }
`

export const Container = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 24px;
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  justify-content: flex-start;
  padding: 16px;
  width: 512px;
  position: relative;
  z-index: 9;
  @media screen and (max-width: 570px) {
    width: 92%;
    padding: 16px 8px;
  }
`

export const Header = styled(StyledDiv)`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  padding: 16px 32px 0px 32px;
  margin-bottom: 45px;
  top: 0;
  width: 100%;
  box-sizing: border-box;
  position: fixed;
  z-index: 10;
  @media screen and (max-width: 570px) {
    padding: 20px 16px 0px 16px;
  }
`

export const BraveLogo = styled(StyledDiv)`
  height: 30px;
  width: 100px;
  background-image: var(--header-icon);
  background-size: cover;
  margin: 0px 12px 4px 0px;
  @media screen and (max-width: 570px) {
    margin: 0px 0px 4px 0px;
  }
`

export const FlipWrapper = styled(StyledDiv)`
  height: 8px;
  width: 100%;
`

export const FlipBox = styled(StyledDiv)`
  position: absolute;
  z-index: 10;
  border-radius: 100%;
  background-color: ${(p) => p.theme.color.background01};
`
