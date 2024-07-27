// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import VPNSvgUrl from '../../assets/svg-icons/vpn-icon.svg'
import checkIconUrl from '../../assets/svg-icons/shield-done.svg'
import sellGraphicUrl from '../../assets/svg-icons/sell-graphic.svg'
import guardianLogoUrl from '../../assets/svg-icons/guardian-logo.svg'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: #0F0663;
  position: relative;
`

export const ProductTitle = styled.h3`
  color: #FFF;
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-weight: 600;
  font-size: 22px;
  font-style: normal;
  margin: 0 0 var(--leo-spacing-xl) 0;
  text-align: center;
  line-height: var(--leo-typography-heading-h3-line-height, 28px);
  letter-spacing: var(--Letter-spacing-Headings, -0.5px);
`

export const PoweredBy = styled.div`
  display: flex;
  align-items: center;

  span {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-style: normal;
    font-weight: 400;
    font-size: 14px;
    color: #FFF;
    text-align: center;
  }
`

export const List = styled.ul`
  align-self: start;
  list-style-type: none;
  padding: 0;
  margin: 0;

  li {
    color: #FFF;
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-weight: 400;
    font-size: 12px;
    font-style: normal;
    line-height: 18px;
    margin-bottom: var(--leo-spacing-m);
    padding-left: 28px;
    text-indent: -28px;
    vertical-align: -6px;

    &:before {
      content: '';
      display: inline-block;
      width: var(--leo-icon-m);
      height: var(--leo-icon-m);
      background-image: url(${checkIconUrl});
      background-repeat: no-repeat;
      background-size: cover;
      user-select: none;
      pointer-events: none;
      margin-right: 8px;
      vertical-align: inherit;
    }

    &:last-child {
      margin-bottom: 0;
    }
  }
`

export const PanelContent = styled.section`
  display: flex;
  padding: var(--leo-spacing-3xl) var(--leo-spacing-2xl);
  flex-direction: column;
  align-items: center;
  gap: var(--leo-spacing-2xl);
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
  box-sizing: border-box;
`

export const SellGraphic = styled.div`
  width: 100%;
  height: 123px;
  background-image: url(${sellGraphicUrl});
  background-repeat: no-repeat;
  background-size: cover;
  position: absolute;
  top: 0;
  user-select: none;
  pointer-events: none;
`

export const MainLogo = styled.div`
  width: var(--leo-spacing-6xl);
  height: var(--leo-spacing-6xl);
  background-image: url(${VPNSvgUrl});
  background-repeat: no-repeat;
  background-size: cover;
  user-select: none;
  pointer-events: none;
`

export const GuardianLogo = styled.i`
  width: 91px;
  height: 20px;
  background-image: url(${guardianLogoUrl});
  background-repeat: no-repeat;
  background-size: cover;
  user-select: none;
  pointer-events: none;
  display: inline-block;
  margin-left: 8px;
`

export const ActionArea = styled.div`
  width: 100%;
  text-align: center;
  box-sizing: border-box;

  a {
    color: #FFF;
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 12px;
    font-style: normal;
    font-weight: 400;
    line-height: 18px;
    text-decoration-line: underline;

    &:focus {
      border: 2px solid var(--color);
      border-radius: 8px;
    }
  }

  button {
    width: 100%;
    min-height: 52px;
    padding: --leo-spacing-l --leo-spacing-xl;
    margin-bottom: var(--leo-spacing-xl);
    border-radius: var(--leo-radius-xl);
    color: #FFF;
    font-size: var(--Size-Button-Large, 15px);
    font-style: normal;
    font-weight: 600;
    line-height: 22px;
    letter-spacing: 0.1px;

    &:first-child {
      backdrop-filter: blur(16px);
      background: rgba(255, 255, 255, 0.24);

      &:hover {
        background: rgba(255, 255, 255, 0.42);
      }
    }
  }
`
