// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { keyframes } from 'brave-ui/theme'

const blurOut = keyframes`
  from {
    filter: blur(20px);
  }
  to {
    filter: blur(0);
  }
`

// The page core structure
export const Section = styled('section')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  min-height: 100vh;
  position: sticky;
  top: 100vh;
  z-index: 3;
  padding: 100px 30px;
  display: flex;
  align-items: center;
  flex-direction: column;
`

export const ArticlesGroup = styled('section')`
  box-sizing: border-box;
  filter: blur(20px);
  animation-name: ${blurOut};
  animation-duration: 0.7s;
  animation-timing-function: ease-out;
  animation-fill-mode: forwards;
`

// Generics

export const Block = styled('div')`
  box-sizing: border-box;
  position: relative;
  width: 680px;
  border-radius: 8px;
  padding: 36px 48px;
  background: rgba(53, 53, 53, 0.47);
  backdrop-filter: blur(23px);
  /* Prevent images from overflowing border-radius */
  overflow-x: hidden;
  margin-bottom: 30px;

  a {
    text-decoration: none;
  }
`

export const Debugger = styled<{}, 'h1'>('h1')`
  background: red;
  position: absolute;
  padding: 5px 0;
  right: 0;
  top: 10px;
  text-transform: uppercase;
`

export const Image = styled('img')`
  box-sizing: border-box;
  max-width: 100%;
  display: block;
`

export const Text = styled<{}, 'p'>('p')`
  box-sizing: border-box;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: normal;
  font-size: 14px;
  line-height: 19px;
  color: #fff;
  margin: 0;
`

export const Heading = styled(Text.withComponent('h2'))`
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: bold;
  font-size: 28px;
  line-height: 36px;
`

export const Time = styled(Text.withComponent('time'))`
  box-sizing: border-box;
  display: block;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 13px;
  margin-top: 4px;
  color: rgba(255,255,255,0.6);
`

export const PublisherLogo = styled<{}, 'img'>('img')`
  box-sizing: border-box;
  max-width: 100%;
  height: 30px;
  min-width: 80px;
  background-color: rgba(188,188,188,0.2);
  margin-top: 12px;
  display: inline-block;

  &::before {
    content: "LOGO PATH MISSING FROM API";
    background: red;
    font-size: 16px;
  }
`
