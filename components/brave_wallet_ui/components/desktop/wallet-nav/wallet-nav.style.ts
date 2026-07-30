// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Navigation from '@brave/leo/react/navigation'
import Button from '@brave/leo/react/button'

// Assets
import WalletLogoLight from '../../../assets/svg-icons/wallet_logo_light.svg'
import WalletLogoDark from '../../../assets/svg-icons/wallet_logo_dark.svg'

// Shared Styles
import { Row } from '../../shared/style'
import {
  layoutSmallWidth,
  navWidth,
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const Wrapper = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  position: absolute;
  background-color: ${leo.color.container.background};
  top: 0px;
  bottom: 0px;
  left: 0px;
  z-index: 10;
  width: ${navWidth}px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    flex-direction: row;
    top: unset;
    left: 0px;
    right: 0px;
    bottom: 0px;
    border: none;
    padding: 8px 0px;
    box-shadow: 0px -8px 16px rgba(0, 0, 0, 0.04);
    width: unset;
    align-items: center;
    justify-content: center;
  }
`

export const SidePanelWrapper = styled.div<{ isOpen: boolean }>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  position: absolute;
  background-color: ${leo.color.container.background};
  top: 0px;
  bottom: 0px;
  left: ${(p) => (p.isOpen ? 0 : -navWidth)}px;
  z-index: 100;
  width: ${navWidth}px;
  transition: all 0.3s ease-in-out;
  border-radius: 0px 16px 16px 0px;
  overflow: hidden;
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

export const WalletLogo = styled.div`
  height: 28px;
  width: 87.65px;
  background-image: url(${WalletLogoLight});
  background-size: cover;
  @media (prefers-color-scheme: dark) {
    background-image: url(${WalletLogoDark});
  }
`

export const LeoNavigation = styled(Navigation)`
  width: 100%;
`

export const Header = styled(Row)`
  background-color: ${leo.color.page.background};
`

export const CloseButton = styled(Button)`
  --leo-button-color: ${leo.color.icon.default};
  flex-grow: 0;
`

export const BackgroundOverlay = styled.div<{ isOpen: boolean }>`
  position: absolute;
  top: 0px;
  left: 0px;
  width: 100%;
  height: 100%;
  backdrop-filter: blur(30px);
  opacity: ${(p) => (p.isOpen ? 1 : 0)};
  transition: all 0.3s ease-in-out;
  z-index: 99;
  pointer-events: ${(p) => (p.isOpen ? 'auto' : 'none')};
`
