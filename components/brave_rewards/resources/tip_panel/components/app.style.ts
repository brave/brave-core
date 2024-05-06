/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

import defaultBackgroundURL from '../assets/default_background.svg'

const panelMinHeight = '500px'

export const root = styled.div`
  font-family: Poppins;
  display: flex;
  justify-content: center;
  position: relative;
  z-index: 1;
  padding: 0 24px;
  overflow: hidden;
  min-height: ${panelMinHeight};
  background: ${leo.color.container.background};

  .narrow-view & {
    flex-direction: column;
  }

  /* Use :where to reduce selector specificity and allow overriding. */
  *:where(a) {
    color: ${leo.color.text.interactive};
    text-decoration: none;

    &:hover {
      text-decoration: underline;
    }
  }
`

export const loading = styled.div`
  height: ${panelMinHeight};
  display: flex;
  align-items: center;
  justify-content: center;
  padding-bottom: 20px;
  color: ${leo.color.text.interactive};

  .icon {
    height: 32px;
    width: auto;

    animation-name: fade-in;
    animation-delay: .5s;
    animation-duration: 1s;
    animation-fill-mode: both;

    @keyframes fade-in {
      from { opacity: 0; }
      to { opacity: .8; }
    }
  }
`

export const error = styled.div`
  font-family: Poppins;
  width: 100%;
  height: ${panelMinHeight};
  padding: 100px 70px;
  background: url(${defaultBackgroundURL});
  background-size: cover;
  display: flex;
  justify-content: center;
`

export const errorCode = styled.div`
  padding-top: 12px;
  font-family: monospace;
  font-size: 11px;
`

export const creator = styled.div`
  flex: 1 1 auto;
  margin: 120px 56px 48px;

  .narrow-view & {
    margin-bottom: 16px;
  }
`

export const form = styled.div`
  flex: 0 0 496px;
  margin: 24px 0 32px;
`
