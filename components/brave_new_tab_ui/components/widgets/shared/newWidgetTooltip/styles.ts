// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import cornerBg from './corner-label-bg.svg'
import plus from './plus.svg'

export const Wrapper = styled<{}, 'aside'>('aside')`
  border-radius: 12px;
  position: relative;
  background: ${p => p.theme.secondary};
  color: #212529;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 11px;
  position: relative;

  &::after {
    content: '';
    width: 0; 
    height: 0; 
    border-left: 20px solid transparent;
    border-right: 20px solid transparent;
    border-top: 20px solid #FFFFFF;
    position: absolute;
    bottom: -20px;
    left: 28px;
  }
`

export const Content = styled<{}, 'div'>('div')`
  padding: 12px 24px 24px;
`

export const Heading = styled<{}, 'div'>('div')`
  padding-bottom: 12px;
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
`

export const Body = styled<{}, 'div'>('div')`
  padding-bottom: 12px;
`

export const CornerLabel = styled<{}, 'div'>('div')`
  max-width: max-content;
  border-radius: 12px 0 0 0;
  background: url(${cornerBg});
  background-size: cover;
  background-position: right;
  background-repeat: no-repeat;
  padding: 4px 24px 4px 12px;
  color: #FFFFFF;
  text-transform: uppercase;
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: 600;
`

export const Button = styled<{}, 'button'>('button')`
  display: block;
  width: 100%;
  background: #FB542B;
  padding: 10px 12px;
  border-radius: 100px;
  border: 1px solid transparent;
  cursor: pointer;
  color: #ffffff;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;
  line-height: 1;
  vertical-align: bottom;

  &::before {
    content: '';
    display: inline-block;
    background: url(${plus});
    background-repeat: no-repeat;
    background-position: left;
    padding-right: 1.5em;
    width: 13px;
    height: 13px;
  }
`