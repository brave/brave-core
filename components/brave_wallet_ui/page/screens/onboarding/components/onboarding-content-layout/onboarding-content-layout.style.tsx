// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

export const StyledWrapper = styled.div`
  position: absolute;
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;

  @media (min-width: 768px) {
    overflow: hidden;
  }
`

export const ContentWrapper = styled.div`
  display: flex;
  position: relative;
  flex-direction: column;
  justify-content: flex-start;
  align-items: center;
  border-radius: ${leo.spacing['2Xl']};
  opacity: 1;
  background: ${leo.color.container.background};
  backdrop-filter: blur(400px);
  width: 100%;
  z-index: 2;
  margin: 100px auto 0;
  padding: ${leo.spacing['2Xl']};

  @media (max-width: 767px) {
    width: 100%;
    padding-left: 12px;
    padding-right: 12px;
    margin: 50px 12px 0 12px;
  }

  @media (min-width: 768px) {
    width: 754px;
  }
`

export const Content = styled.div`
  display: flex;
  justify-content: center;
  flex-direction: column;
  width: 100%;

  @media (min-width: 500px) {
    width: 450px;
  }
`

export const Title = styled.h3`
  color: ${leo.color.text.primary};
  text-align: center;
  font-family: Poppins;
  font-size: 32px;
  font-style: normal;
  font-weight: 500;
  margin: 0;
  padding: 0;
`

export const Subtitle = styled.p`
  color: ${leo.color.text.secondary};
  text-align: center;
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 400;
  letter-spacing: -0.2px;
  margin: 0;
  padding: 0;
`

export const BackButton = styled.button`
  display: flex;
  width: 36px;
  min-height: 36px;
  padding: ${leo.spacing.m} ${leo.spacing.l};
  justify-content: center;
  align-items: center;
  border-radius: ${leo.spacing.m};
  border: 1px solid ${leo.color.divider.interactive};
  background: ${leo.color.container.background};
  cursor: pointer;
`

export const BackButtonIcon = styled(Icon).attrs({
  name: 'arrow-left'
})`
  color: ${leo.color.icon.interactive};
  --leo-icon-size: 18px;
`

export const StaticBackground = styled.div`
  position: fixed;
  top: 0px;
  bottom: 0px;
  left: 0px;
  right: 0px;
  background-color: ${leo.color.page.background};
  z-index: 0;
`

export const BackgroundGradientBottomLayer = styled.div`
  position: absolute;
  left: 15%;
  right: 15%;
  top: 65%;
  bottom: 0px;
  border-radius: 100%;
  background: rgba(158, 169, 240, 0.3);
  opacity: 1;
  filter: blur(190px);
  z-index: 1;
  @media (prefers-color-scheme: dark) {
    top: 75%;
    background: rgba(78, 76, 158, 0.8);
  }
`

export const BraveIcon = styled(Icon).attrs({
  name: 'brave-icon-release-color'
})`
  --leo-icon-size: 20px;
`

export const WalletTitle = styled.h4`
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 600;
  line-height: 26px;
  color: ${leo.color.text.primary};
  margin: 0;
`

export const TitleSection = styled.div`
  display: flex;
  position: absolute;
  left: ${leo.spacing.xl};
  top: 0;
  justify-content: flex-start;
  align-items: center;
  width: 100%;
  padding: ${leo.spacing.xl};
`
