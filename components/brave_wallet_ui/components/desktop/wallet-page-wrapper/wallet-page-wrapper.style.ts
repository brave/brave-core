// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import { Row } from '../../shared/style'

const minCardHeight = 497
const maxCardWidth = 768
const layoutScaleWithNav = 1374
const layoutSmallCardBottom = 67
export const layoutSmallWidth = 980
export const layoutPanelWidth = 660
export const layoutTopPosition = 68

export const Wrapper = styled.div<{
  noPadding?: boolean
  isPanel?: boolean
}>`
  --layout-top-position: ${(p) => (p.isPanel ? 0 : layoutTopPosition)}px;
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
  z-index: 10;
  padding: ${(p) => (p.noPadding ? '0px' : `var(--layout-top-position) 0px`)};
`

export const LayoutCardWrapper = styled.div<{
  hideNav?: boolean
  hideCardHeader?: boolean
  headerHeight: number
}>`
  --header-top-position: calc(
    var(--layout-top-position) + ${(p) => p.headerHeight}px
  );
  --no-header-top-position: var(--layout-top-position);
  --top-position: ${(p) =>
    p.hideCardHeader
      ? 'var(--no-header-top-position)'
      : 'var(--header-top-position)'};
  --bottom-position: ${(p) => (p.hideNav ? 0 : layoutSmallCardBottom)}px;
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: center;
  top: var(--top-position);
  bottom: 0px;
  position: absolute;
  width: 100%;
  overflow-x: hidden;
  overflow-y: auto;
  padding-bottom: 32px;
  &::-webkit-scrollbar {
    display: none;
  }
  @media screen and (max-width: ${layoutScaleWithNav}px) {
    padding: 0px 32px 32px 304px;
    align-items: flex-start;
  }
  @media screen and (max-width: ${layoutSmallWidth}px) {
    bottom: var(--bottom-position);
    padding: 0px 32px 32px 32px;
    align-items: center;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px;
  }
`

export const ContainerCard = styled.div<{
  noPadding?: boolean
  maxWidth?: number
  hideCardHeader?: boolean
  noMinCardHeight?: boolean
  noBorderRadius?: boolean
  useDarkBackground?: boolean
}>`
  display: flex;
  flex: none;
  flex-direction: column;
  background-color: ${(p) =>
    p.useDarkBackground
      ? leo.color.page.background
      : leo.color.container.background};
  border-radius: ${(p) => (p.hideCardHeader ? '24px' : '0px 0px 24px 24px')};
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.07);
  box-sizing: border-box;
  justify-content: flex-start;
  align-items: center;
  padding: ${(p) => (p.noPadding ? 0 : 20)}px;
  width: 100%;
  min-height: ${(p) => (p.noMinCardHeight ? 'unset' : `${minCardHeight}px`)};
  max-width: ${(p) => (p.maxWidth ? p.maxWidth : maxCardWidth)}px;
  position: relative;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    max-width: ${(p) => (p.maxWidth ? `${p.maxWidth}px` : 'unset')};
    width: 100%;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    min-height: calc(100vh - var(--bottom-position) - var(--top-position));
    border-radius: ${(p) =>
      p.noBorderRadius
        ? '0px'
        : p.hideCardHeader
        ? '24px 24px 0px 0px'
        : '0px'};
  }
`

export const CardHeaderWrapper = styled.div<{
  maxWidth?: number
  isPanel?: boolean
}>`
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: center;
  top: var(--layout-top-position);
  position: fixed;
  width: 100%;
  max-width: ${(p) => (p.maxWidth ? `${p.maxWidth}px` : 'unset')};
  @media screen and (max-width: ${layoutScaleWithNav}px) {
    padding: ${(p) => (p.maxWidth ? '0px' : '0px 32px 0px 304px')};
    left: ${(p) => (p.maxWidth ? 'unset' : '0px')};
    right: ${(p) => (p.maxWidth ? 'unset' : '0px')};
    align-items: flex-start;
  }
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: unset;
    right: unset;
    align-items: center;
    padding: ${(p) => (p.maxWidth ? '0px' : '0px 32px')};
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px;
    z-index: ${(p) => (p.isPanel ? 10 : 'unset')};
  }
`

export const CardHeader = styled.div<{
  shadowOpacity?: number
  isPanel?: boolean
  useDarkBackground?: boolean
}>`
  --shadow-opacity: ${(p) =>
    p.shadowOpacity !== undefined ? p.shadowOpacity : 0};
  display: flex;
  background-color: ${(p) =>
    p.useDarkBackground
      ? leo.color.page.background
      : leo.color.container.background};
  border-radius: ${(p) => (p.isPanel ? '0px' : '24px 24px 0px 0px')};
  width: 100%;
  padding: ${(p) => (p.isPanel ? '0px' : '0px 32px')};
  position: relative;
  max-width: ${maxCardWidth}px;
  box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, var(--shadow-opacity));
  @media screen and (max-width: ${layoutSmallWidth}px) {
    max-width: unset;
  }
`

export const CardHeaderShadow = styled(CardHeader)<{
  headerHeight: number
}>`
  height: ${(p) => p.headerHeight}px;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.07);
`

export const CardHeaderContentWrapper = styled(Row)<{
  dividerOpacity?: number
  hideDivider?: boolean
}>`
  --divider-opacity: ${(p) =>
    p.dividerOpacity !== undefined ? p.dividerOpacity : 1};
  --divider-color: rgba(232, 233, 238, var(--divider-opacity));
  @media (prefers-color-scheme: dark) {
    --divider-color: rgba(43, 46, 59, var(--divider-opacity));
  }
  border-bottom: ${(p) =>
    p.hideDivider ? 'none' : '1px solid var(--divider-color)'};
  height: 100%;
`

export const StaticBackground = styled.div`
  position: fixed;
  top: 0px;
  bottom: 0px;
  left: 0px;
  right: 0px;
  background-color: ${leo.color.page.background};
`

export const BackgroundGradientWrapper = styled.div`
  position: fixed;
  top: 0px;
  bottom: 0px;
  left: 0px;
  right: 0px;
  opacity: 0.5;
  background-color: ${leo.color.page.background};
`

export const BackgroundGradientTopLayer = styled.div`
  position: absolute;
  left: -42%;
  right: 35%;
  top: 15%;
  bottom: 25%;
  background: #dfdefc;
  border-radius: 100%;
  filter: blur(36.2567px);
  transform: matrix(1, -0.06, -0.32, -0.95, 0, 0);
  z-index: 5;
  @media (prefers-color-scheme: dark) {
    /* #013F4B does not exist in design system */
    background: #013f4b;
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
  background: #d6e7ff;
  border-radius: 100%;
  filter: blur(47.5869px);
  transform: matrix(-1, 0.06, -0.32, -0.95, 0, 0);
  z-index: 4;
  @media (prefers-color-scheme: dark) {
    /* #030A49 does not exist in design system */
    background: #030a49;
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
  background: #c8edfd;
  border-radius: 100%;
  filter: blur(47.5869px);
  transform: matrix(-1, 0.06, -0.32, -0.95, 0, 0);
  z-index: 3;
  @media (prefers-color-scheme: dark) {
    /* #014B3A does not exist in design system */
    background: #014b3a;
    filter: blur(70px);
    left: 25%;
    right: -80%;
    top: 15%;
    bottom: -40%;
    transform: matrix(-0.79, 0.61, -0.98, -0.22, 0, 0);
  }
`

export const BlockForHeight = styled.div`
  top: var(--layout-top-position);
  width: 1px;
  height: calc(${minCardHeight}px + 30px);
  display: flex;
  position: absolute;
  left: 0px;
`

export const FeatureRequestButtonWrapper = styled.div`
  display: flex;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    display: none;
  }
`
