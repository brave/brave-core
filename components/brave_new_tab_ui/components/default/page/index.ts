/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'

export const Page = styled<{}, 'div'>('div')`
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
  padding: 80px 60px 40px;
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
`

export const Gradient = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  background: linear-gradient(#000000, rgba(0, 0, 0, 0));
  height: 400px;
`

export const Link = styled<{}, 'a'>('a')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  font-weight: 400;
  color: #fff;
  font-size: 13px;
  text-transform: uppercase;
  text-decoration: underline;
`

export const PhotoName = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  -webkit-font-smoothing: antialiased;
  font-weight: 400;
  color: rgba(255, 255, 255, 0.5);
  font-size: 23px;
`

export const Navigation = styled<{}, 'nav'>('nav')`
  display: flex;
`

export const IconLink = styled<{}, 'div'>('div')`
  box-sizing: border-box;
  display: flex;
  width: 35px;
  height: 35px;
  margin: 15px 5px;
  cursor: default;
  color: #fff;
`
