// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import {
  layoutSmallWidth,
  layoutTopPosition
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const Wrapper = styled.div<{
  isPanel: boolean
}>`
  --nav-background: ${leo.color.container.background};
  @media (prefers-color-scheme: dark) {
    /* #1C2026 does not exist in design system */
    --nav-background: ${(p) =>
      p.isPanel ? '#1C2026' : leo.color.container.background};
  }
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: var(--nav-background);
  border-radius: 16px;
  position: absolute;
  top: ${layoutTopPosition}px;
  left: 32px;
  overflow: visible;
  z-index: 10;
  width: 240px;
  padding: 12px 0px;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.07);
  @media screen and (max-width: ${layoutSmallWidth}px) {
    flex-direction: row;
    top: unset;
    left: 0px;
    right: 0px;
    bottom: 0px;
    border: none;
    padding: 8px 0px;
    border-radius: 0px;
    box-shadow: 0px -8px 16px rgba(0, 0, 0, 0.04);
    width: unset;
  }
`

export const Section = styled.div<{ showBorder?: boolean }>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
  padding: 12px 0px;
  border-bottom: ${(p) =>
    p.showBorder ? `1px solid ${leo.color.container.highlight}` : 'none'};
  @media screen and (max-width: ${layoutSmallWidth}px) {
    flex-direction: row;
    padding: 0px 8px;
    border-bottom: none;
    border-right: ${(p) =>
      p.showBorder ? `1px solid ${leo.color.container.highlight}` : 'none'};
  }
`

export const PageOptionsWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    display: none;
  }
`

export const PanelOptionsWrapper = styled.div`
  display: none;
  width: 100%;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    display: flex;
  }
`
