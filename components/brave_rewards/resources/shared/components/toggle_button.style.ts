/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  --self-height: var(--toggle-button-height, 24px);
  --self-width:  var(--toggle-button-width, 48px);
  --self-handle-margin: var(--toggle-button-handle-margin, 3px);
  --self-handle-size: calc(var(--self-height) - var(--self-handle-margin) * 2);

  button {
    position: relative;
    height: var(--self-height);
    width: var(--self-width);
    background: #ACAFBB;
    border: none;
    border-radius: 32px;
    margin: 0;
    padding: 0;
    vertical-align: middle;
    cursor: pointer;

    @media (prefers-color-scheme: dark) {
      background: #585C6D;
    }

    .brave-theme-dark & {
      background: #585C6D;
    }

    &.checked {
      background: var(--toggle-button-color, #4C54D2);
    }
  }
`

export const handle = styled.div`
  position: absolute;
  top: var(--self-handle-margin);
  left: var(--self-handle-margin);
  right: var(--self-handle-margin);
  background: #fff;
  height: var(--self-handle-size);
  width: var(--self-handle-size);
  border-radius: 50%;
  transition: all .3s ease;

  button.checked & {
    right: 0;
    left: calc(100% - var(--self-handle-size) - var(--self-handle-margin));
    right: calc(100% - var(--self-handle-size) - var(--self-handle-margin));
  }
`
