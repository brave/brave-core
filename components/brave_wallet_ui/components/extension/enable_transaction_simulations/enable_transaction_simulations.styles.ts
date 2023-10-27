// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/'

export const errorIconColor = leo.color.systemfeedback.errorIcon

export const Header = styled.div`
  width: 100%;
  min-height: 136px;
  padding: 40px 62px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  border-top-left-radius: 8px;
  border-top-right-radius: 8px;
  background-color: ${leo.color.page.background};
`

export const IconContainer = styled.div<{
  iconColor?: string
}>`
  --leo-icon-size: 20px;
  --leo-icon-color: ${(p) => p.iconColor || 'unset'};
  width: 56px;
  height: 56px;
  padding: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 6px;
  background-color: ${leo.color.container.background};
`

export const DashedHorizontalLine = styled.div`
  display: inline-block;
  width: 46px;
  color: ${leo.color.divider.strong};
  border: 1px ${leo.color.divider.strong} dashed;
`

export const CardContent = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  padding-top: 24px;
  padding-left: 24px;
  padding-right: 24px;
`

export const HeadingText = styled.p`
  padding: 0px 26px;
  text-align: center;
  margin: 0;
  color: ${leo.color.text.primary};
  font-size: 16px;
  font-family: Poppins;
  font-weight: 600;
  line-height: 24px;
`

export const BulletPoints = styled.ul`
  text-align: left;
  padding-left: 38px;
  & > li {
    margin-bottom: 4px;
    font-size: 12px;
    font-family: Poppins;
    line-height: 18px;
    line-height: 18px;
    color: ${leo.color.text.primary};
  }
`

export const TermsText = styled.p`
  width: 100%;
  margin-bottom: 4px;
  text-align: left;
  font-size: 11px;
  font-family: Poppins;
  line-height: 16px;
  color: ${leo.color.text.tertiary};
  & > strong {
    color: ${leo.color.text.secondary};
  }
`

export const LearnMoreLink = styled.a`
  line-height: 16px;
  text-decoration: none;
  color: ${leo.color.text.interactive};
  font: ${leo.font.xSmall.regular};
`

export const OptionsRow = styled.div`
  display: flex;
  flex-direction: row;
  width: 100%;
  padding: 16px 0px;
  align-items: center;
  justify-content: center;
  gap: 8px;

  & > * {
    flex-basis: 50%;
  }
`
