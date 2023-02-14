/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import '@brave/leo/tokens/css/variables.css'

export const root = styled.div`
  font: var(--font-text-default-regular);
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  position: relative;
  z-index: 1;
  padding: 0 24px;

  /* Use :where to reduce selector specificity and allow overriding. */
  *:where(a) {
    color: var(--color-text-interactive);
    text-decoration: none;

    &:hover {
      text-decoration: underline;
    }
  }
`

export const loading = styled.div`
  min-height: 400px;
  display: flex;
  align-items: center;
  padding-bottom: 20px;
  color: var(--color-text-interactive);

  .icon {
    height: 32px;
    width: auto;

    animation-name: fade-in;
    animation-delay: 1s;
    animation-duration: 1s;
    animation-fill-mode: both;

    @keyframes fade-in {
      from { opacity: 0; }
      to { opacity: .8; }
    }
  }
`

export const creator = styled.div`
  flex: 1 1 300px;
  margin: 120px 56px 32px;
`

export const form = styled.div`
  flex: 0 0 496px;
  margin: 24px 0 32px;
`
