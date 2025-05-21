/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    position: relative;
    container-type: inline-size;
  }

  .items {
    overflow-x: auto;
    overflow-y: hidden;
    scroll-snap-type: x mandatory;
    scrollbar-width: none;
    display: flex;
    gap: 16px;
  }

  button {
    --leo-icon-size: 24px;

    position: absolute;
    inset-block-start: 24px;
    background: #fff;
    border-radius: 50%;
    box-shadow: 0px 1px 4px rgba(63, 76, 99, 0.35);
    padding: 4px;
    visibility: hidden;
    opacity: 0;
    transition: opacity 120ms;
    color: ${color.icon.secondary};

    &.left {
      left: -12px;
    }

    &.right {
      right: -12px;
    }

    &:hover {
      box-shadow: 0px 1px 4px rgba(63, 76, 99, 0.5);
    }
  }

  &:dir(ltr):hover {
    .can-scroll-back ~ button.left {
      opacity: 1;
      visibility: visible;
    }

    .can-scroll-forward ~ button.right {
      opacity: 1;
      visibility: visible;
    }
  }

  &:dir(rtl):hover {
    .can-scroll-back ~ button.right {
      opacity: 1;
      visibility: visible;
    }

    .can-scroll-forward ~ button.left {
      opacity: 1;
      visibility: visible;
    }
  }
`

style.passthrough.css`
  .items > * {
    min-width: calc(33.3333cqi - 10.6667px);
    max-width: 208px;
    scroll-snap-align: start;
  }
`
