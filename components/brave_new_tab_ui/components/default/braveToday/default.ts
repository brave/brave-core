// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled, { css, keyframes } from 'styled-components'

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
  z-index: 5;
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  min-height: 100vh;
  margin: 100px 30px;
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
  overflow: hidden;
  box-sizing: border-box;
  position: relative;
  width: 680px;
  border-radius: 8px;
  padding: 36px 48px;
  background: rgba(53, 53, 53, 0.47);
  backdrop-filter: blur(62px);
  margin-bottom: 30px;
  color: white;
  a {
    text-decoration: none;
  }
`

export const Debugger = styled('h1')<{}>`
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

export const Text = styled('div')<{}>`
  box-sizing: border-box;
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: normal;
  font-size: 14px;
  line-height: 19px;
  color: #fff;
  margin: 0;
`

export const Heading = styled(Text.withComponent('h2'))`
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: 600;
  font-size: 22px;
  line-height: 32px;
`

export const Time = styled(Text.withComponent('time'))`
  box-sizing: border-box;
  display: block;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 13px;
  margin-top: 4px;
  color: rgba(255,255,255,0.7);
`

export const PublisherLogo = styled('img')<{}>`
  box-sizing: border-box;
  max-width: 100%;
  height: 30px;
  min-width: 80px;
  background-color: rgba(188,188,188,0.2);
  margin-top: 12px;
  display: inline-block;
`

export const Button = styled('button')<{}>`
  appearance: none;
  cursor: pointer;
  display: block;
  border-radius: 24px;
  background: none;
  padding: 15px 34px;
  color: white;
  border: none;
  font-weight: 800;
  cursor: pointer;
  background: rgba(33, 37, 41, .8);
  backdrop-filter: blur(8px);
  outline: none;
  border: none;
  transition: opacity 1s ease-in-out, background .124s ease-in-out;
  &:hover {
    background: rgba(255, 255, 255, .2);
  }
  &:active {
    background: rgba(255, 255, 255, .4);
  }
  &:focus-visible {
    box-shadow: 0 0 0 1px ${p => p.theme.color.brandBrave};
  }
`

type CardButtonProps = {
  isMainFocus?: boolean
}
export const CardButton = styled(Button)<CardButtonProps>`
  backdrop-filter: none;
  background: rgba(255, 255, 255, .24);
  ${p => p.isMainFocus && css`
    display: block;
    width: 100%;
  `}
`

export const TertiaryButton = styled('button')<{}>`
  appearance: none;
  cursor: pointer;
  font-family: Poppins;
  font-weight: 600;
  font-size: 13px;
  background: none;
  border: none;
  outline: none;
  margin: 0;
  padding: 0;
  color: white;
  &:hover {
    color: #ddd;
  }
  &:active {
    transform: translate(1px, 1px)
  }
  &:focus-visible {
    box-shadow: 0 0 0 1px ${p => p.theme.color.brandBrave};
  }
`
