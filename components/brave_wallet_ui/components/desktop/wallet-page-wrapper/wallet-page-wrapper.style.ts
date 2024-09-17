// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Assets
import TopLayerLight from './assets/top_layer_light.svg'
import BottomLayerLight from './assets/bottom_layer_light.svg'
import TopLayerDark from './assets/top_layer_dark.svg'
import BottomLayerDark from './assets/bottom_layer_dark.svg'
import LinesLight from './assets/portfolio_lines_background_light.svg'
import LinesDark from './assets/portfolio_lines_background_dark.svg'

// Shared Styles
import { Row } from '../../shared/style'

const minCardHeight = 497
export const maxCardWidth = 768
const layoutSmallCardBottom = 67
export const layoutSmallWidth = 1100
export const layoutPanelWidth = 660
export const layoutTopPosition = 68
// navSpace and navWidth need to be defined here to prevent circular imports.
export const navWidth = 250

export const Wrapper = styled.div<{
  noPadding?: boolean
  noTopPosition?: boolean
}>`
  --layout-top-position: ${(p) => (p.noTopPosition ? 0 : layoutTopPosition)}px;
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
  padding?: string
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
  /*
    (100vw / 2) - (${maxCardWidth}px / 2) makes the card body perfectly centered
    horizontally in the browser window.
  */
  --left-padding-without-nav: calc((100vw / 2) - (${maxCardWidth}px / 2));
  /*
    + (${navWidth}px / 2) is to then adjust the card body
    to the right to be centered with the nav.
  */
  --left-padding-with-nav: calc(
    var(--left-padding-without-nav) + (${navWidth}px / 2)
  );
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: flex-start;
  top: var(--top-position);
  bottom: 0px;
  position: absolute;
  width: 100%;
  overflow-x: hidden;
  overflow-y: auto;
  padding: ${(p) => p.padding ?? '0px 0px 32px 0px'};
  &::-webkit-scrollbar {
    display: none;
  }
  padding-left: ${(p) =>
    p.hideNav
      ? 'var(--left-padding-without-nav)'
      : 'var(--left-padding-with-nav)'};
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
  hideCardHeader?: boolean
  noMinCardHeight?: boolean
  noBorderRadius?: boolean
  useDarkBackground?: boolean
  useFullHeight?: boolean
  noBackground?: boolean
}>`
  display: flex;
  flex: none;
  flex-direction: column;
  background-color: ${(p) =>
    p.noBackground
      ? 'unset'
      : p.useDarkBackground
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
  height: ${(p) =>
    p.useFullHeight ? 'calc(100vh - var(--top-position))' : 'unset'};
  max-width: ${maxCardWidth}px;
  position: relative;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    max-width: unset;
    width: 100%;
    height: ${(p) =>
      p.useFullHeight
        ? 'calc(100vh - var(--top-position) - var(--bottom-position))'
        : 'unset'};
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
  isPanel?: boolean
}>`
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: flex-start;
  top: var(--layout-top-position);
  position: fixed;
  width: 100%;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: unset;
    right: unset;
    align-items: center;
    padding: 0px 32px;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px;
    z-index: ${(p) => (p.isPanel ? 10 : 'unset')};
  }
`

export const CardHeader = styled.div<{
  shadowOpacity?: number
  backgroundOpacity?: number
  isPanel?: boolean
  isAndroid?: boolean
  useDarkBackground?: boolean
}>`
  --shadow-opacity: ${(p) =>
    p.shadowOpacity !== undefined ? p.shadowOpacity : 0};
  --background-opacity: ${(p) =>
    p.backgroundOpacity !== undefined ? p.backgroundOpacity : 0};
  // Needed to extract the rgb values from
  // leo.color.container.background since hex
  // does not work for this needed effect.
  --header-background: 255, 255, 255;
  @media (prefers-color-scheme: dark) {
    --header-background: 13, 15, 20;
  }
  --dark-background-color: rgba(
    var(--header-background),
    var(--background-opacity)
  );
  display: flex;
  background-color: ${(p) =>
    p.useDarkBackground
      ? 'var(--dark-background-color)'
      : leo.color.container.background};
  border-radius: ${(p) =>
    p.isPanel || p.isAndroid ? '0px' : '24px 24px 0px 0px'};
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
  opacity: 0.3;
  background-color: ${leo.color.primary[10]};
`

export const BackgroundGradientTopLayer = styled.div`
  width: 100%;
  height: 100%;
  background-image: url(${TopLayerLight});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: center;
  position: absolute;
  z-index: 4;
  @media (prefers-color-scheme: dark) {
    background-image: url(${TopLayerDark});
  }
`

export const BackgroundGradientBottomLayer = styled.div`
  width: 100%;
  height: 100%;
  background-image: url(${BottomLayerLight});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: center;
  position: absolute;
  z-index: 5;
  @media (prefers-color-scheme: dark) {
    background-image: url(${BottomLayerDark});
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
  @media screen and (max-width: 1445px) {
    display: none;
  }
`

export const PortfolioBackgroundWatermark = styled.div`
  width: 100%;
  height: 100%;
  background-image: url(${LinesLight});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: center;
  position: absolute;
  top: 0px;
  left: 0px;
  @media (prefers-color-scheme: dark) {
    background-image: url(${LinesDark});
  }
`
