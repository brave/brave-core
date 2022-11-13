// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import backgroundDarkURL from '../../assets/background_dark.svg'
import backgroundLightURL from '../../assets/background_light.svg'
import animationLightURL from '../../assets/animation_light.gif'
import animationDarkURL from '../../assets/animation_dark.gif'
import cookieImageURL from '../../assets/cookie.png'

export const Root = styled.div`
  --available-height: 800px;

  --color-link: #4C54D2;
  --color-titlebar-background: #F8F9FA;
  --color-titlebar-text: #3B3E4F;
  --color-content-background: #fff;
  --color-content-text: #212529;
  --color-decline: var(--color-link);
  --background-image-url: url(${backgroundLightURL});
  --animation-image-url: url(${animationLightURL});

  @media (prefers-color-scheme:dark) {
    --color-link: #737ADE;
    --color-titlebar-background: #313341;
    --color-titlebar-text: #E9E9F4;
    --color-content-background: #1E2029;
    --color-content-text: #F0F2FF;
    --color-decline: #FFFFFF;
    --background-image-url: url(${backgroundDarkURL});
    --animation-image-url: url(${animationDarkURL});
  }

  max-height: var(--available-height);
  overflow-y: auto;
  position: relative;
  font-family: Poppins;

  a {
    color: var(--color-link);
    text-decoration: none;
  }
`

export const TitleBar = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  padding: 16px 24px;
  gap: 18px;

  background: var(--color-titlebar-background);

  svg {
    vertical-align: middle;
  }

  button {
    margin: 0;
    padding: 0;
    border: none;
    background: none;
    cursor: pointer;
  }
`

export const TitleBarText = styled.div`
  flex: 1 1 auto;
  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  color: var(--color-titlebar-text);
`

export const Content = styled.div`
  min-height: 560px;
  background-color: var(--color-content-background);
  background-image: var(--background-image-url);
  background-position: center bottom;
  background-size: cover;
  background-repeat: no-repeat;
  text-align: center;
  color: var(--color-content-text);
  padding-top: 75px;
  padding-bottom: 46px;

  display: flex;
  flex-direction: column;

  .success & > * {
    visibility: hidden;
  }
`

export const CookieGraphic = styled.div`
  flex: none;
  height: 48px;
  background-image: url(${cookieImageURL});
  background-repeat: no-repeat;
  background-position: center;
  background-size: contain;

  .success & {
    visibility: visible;
  }
`

export const Header = styled.div`
  flex: none;
  margin: 54px 30px 0;
  font-weight: 500;
  font-size: 24px;
  line-height: 36px;
`

export const Description = styled.div`
  flex: 1 0 auto;
  margin: 40px 30px 0;
  font-weight: 500;
  font-size: 14px;
  line-height: 26px;
  letter-spacing: 0.01em;
`

export const OptIn = styled.div`
  flex: none;
  margin: 40px 40px 0;
`

export const Decline = styled.div`
  flex: none;
  margin: 25px 0 0;

  button {
    color: var(--color-decline);
    border: none;
    background: none;
    margin: 0;
    padding: 0;
    font-weight: 600;
    font-size: 14px;
    line-height: 14px;
    cursor: pointer;
  }
`

export const Animation = styled.div`
  position: absolute;
  top: 74px;
  left: 0;
  right: 0;
  height: 388px;
  z-index: 0;
  background-image: var(--animation-image-url);
  background-repeat: no-repeat;
  background-position: 0.5px 0.5px;
  background-size: auto 380px;
`
