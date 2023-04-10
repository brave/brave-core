// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

const minCardHeight = 595

export const Wrapper = styled.div<{ noPadding?: boolean }>`
  position: fixed;
  top: 0px;
  bottom: 0px;
  left: 0px;
  right: 0px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  overflow: hidden;
  overflow-y: auto;
  z-index: 10;
  padding: ${(p) => p.noPadding ? '0px' : '100px 0px'};
`

export const LayoutCardWrapper = styled.div<{
  maxWidth?: number,
}>`
  display: flex;
  flex-direction: column;
  box-sizing: border-box;
  justify-content: flex-start;
  align-items: center;
  top: 100px;
  bottom: 32px;
  position: absolute;
  width: 100%;
  max-width: ${(p) => p.maxWidth ? p.maxWidth : 700}px;
`

export const ContainerCard = styled.div<
  {
    cardOverflow?: 'auto' | 'hidden' | 'visible'
  }>`
  display: flex;
  flex-direction: column;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 24px;
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  justify-content: flex-start;
  align-items: center;
  padding: 20px;
  width: 100%;
  min-height: ${minCardHeight}px;
  max-height: calc(100vh - 132px);
  overflow-y: ${(p) => p.cardOverflow ?? 'hidden'};
  position: relative;
  @media screen and (max-width: 700px) {
    width: 100%;
  }
`

export const StaticBackground = styled.div`
  position: fixed;
  top: 0px;
  bottom: 0px;
  left: 0px;
  right: 0px;
  background-color: ${leo.color.container.highlight};
  @media (prefers-color-scheme: dark) {
    /* #17171F does not exist in design system */
    background-color: #17171F;
  }
`

export const BackgroundGradientWrapper = styled.div`
  position: fixed;
  top: 0px;
  bottom: 0px;
  left: 0px;
  right: 0px;
  opacity: 0.5;
  background-color: ${leo.color.container.highlight};
`

export const BackgroundGradientTopLayer = styled.div`
  position: absolute;
  left: -42%;
  right: 35%;
  top: 15%;
  bottom: 25%;
  background: #DFDEFC;
  border-radius: 100%;
  filter: blur(36.2567px);
  transform: matrix(1, -0.06, -0.32, -0.95, 0, 0);
  z-index: 5;
  @media (prefers-color-scheme: dark) {
    /* #013F4B does not exist in design system */
    background: #013F4B;
    filter: blur(47px);
    left: 35%;
    right: -100%;
    top: 2%;
    bottom: 25%;
    transform: matrix(-0.98, 0.19, -0.73, -0.68, 0, 0);
  }
`

export const BackgroundGradientMiddleLayer = styled.div`
  position: absolute;
  left: 25%;
  right: 10%;
  top: 10%;
  bottom: 25%;
  background: #D6E7FF;
  border-radius: 100%;
  filter: blur(47.5869px);
  transform: matrix(-1, 0.06, -0.32, -0.95, 0, 0);
  z-index: 4;
  @media (prefers-color-scheme: dark) {
    /* #030A49 does not exist in design system */
    background: #030A49;
    filter: blur(70px);
    left: -40%;
    right: 17%;
    top: 50%;
    bottom: -80%;
    transform: matrix(-0.98, 0.19, -0.73, -0.68, 0, 0);
  }
`

export const BackgroundGradientBottomLayer = styled.div`
  position: absolute;
  left: -30%;
  right: 20%;
  top: 45%;
  bottom: -25%;
  background: #C8EDFD;
  border-radius: 100%;
  filter: blur(47.5869px);
  transform: matrix(-1, 0.06, -0.32, -0.95, 0, 0);
  z-index: 3;
  @media (prefers-color-scheme: dark) {
    /* #014B3A does not exist in design system */
    background: #014B3A;
    filter: blur(70px);
    left: 25%;
    right: -80%;
    top: 15%;
    bottom: -40%;
    transform: matrix(-0.79, 0.61, -0.98, -0.22, 0, 0);
  }
`

export const BlockForHeight = styled.div`
  top: 100px;
  width: 1px;
  height: calc(${minCardHeight}px + 30px);
  display: flex;
  position: absolute;
  left: 0px;
`
