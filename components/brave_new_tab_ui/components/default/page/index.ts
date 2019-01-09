/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { keyframes } from '../../../../theme'

const fadeIn = keyframes`
  from {
    opacity: 0;
  }
  to {
    opacity: 1
  }
`

export const Page = styled<{}, 'div'>('div')`
  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  position: relative;
  z-index: 2;
  top: 0;
  left: 0;
  display: flex;
  flex: 1;
  flex-direction: column;
  justify-content: space-between;
  height: 100%;
  min-height: 100vh;
`

interface DynamicBackgroundProps {
  background: string
}

export const DynamicBackground = styled<DynamicBackgroundProps, 'div'>('div')`
  box-sizing: border-box;
  background-position: top center;
  background-repeat: no-repeat;
  background-size: cover;
  background-image: url(${p => p.background});
  display: flex;
  flex: 1;
  opacity: 0;
  animation: ${fadeIn} 300ms;
  animation-fill-mode: forwards;
`

export const Gradient = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  background: linear-gradient(#000000, rgba(0, 0, 0, 0));
  height: 700px;
`

export const Link = styled<{}, 'a'>('a')`
  font-weight: 800;
  text-decoration: none;
  transition: color 0.15s ease, filter 0.15s ease;
  color: rgba(255, 255, 255, 0.6);

  &:hover {
    color: rgba(255, 255, 255, 1);
  }
`

export const PhotoName = styled<{}, 'div'>('div')`
  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  font-size: 12px;
  font-family: Muli, sans-serif;
  color: rgba(255, 255, 255, 0.6);
`

export const Navigation = styled<{}, 'nav'>('nav')`
  display: flex;
`

export const IconLink = styled<{}, 'a'>('a')`
  display: flex;
  width: 24px;
  height: 24px;
  margin: 12px;
  cursor: pointer;
  color: #FFFFFF;
  opacity: 0.7;
  transition: opacity 0.15s ease, filter 0.15s ease;

  &:hover {
    opacity: 0.95;
  }
`
