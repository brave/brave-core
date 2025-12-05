/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'
import { color } from '@brave/leo/tokens/css/variables'

import thumbOnBackground from '../assets/toggle_thumb_on.svg'
import thumbOffBackground from '../assets/toggle_thumb_off.svg'

export const style = scoped.css`
  :scope {
    position: relative;
    width: 64px;
    height: 40px;

    border-radius: 100px;
    border: 1px solid ${color.primitive.primary[25]};
    background:
      linear-gradient(
        0deg,
        rgba(0, 0, 0, 0.20) 0%,
        rgba(0, 0, 0, 0.20) 100%),
      linear-gradient(
        174deg,
        ${color.primitive.primary[40]} 2.32%,
        ${color.primitive.primary[35]} 93.33%);

    box-shadow: 0 8px 8px 0 ${color.primitive.primary[25]} inset;
  }

  :scope[data-active=false] {
    border-radius: 100px;
    border: 1px solid ${color.primitive.neutral[70]};
    background:
      linear-gradient(
        174deg,
        ${color.primitive.neutral[80]} 2.32%,
        ${color.primitive.neutral[70]} 93.33%);

    box-shadow: 0 2px 4px 0 ${color.primitive.neutral[70]} inset;
  }

  .thumb {
    --self-thumb-size: 38px;

    position: absolute;
    inset-block: 0;
    inset-inline-start: calc(100% - var(--self-thumb-size));
    inset-inline-end: auto;
    width: var(--self-thumb-size);

    fill: linear-gradient(
      180deg,
      ${color.primitive.primary[100]} 26.44%,
      ${color.primitive.primary[80]} 100%);

    stroke-width: 1px;
    stroke: ${color.primitive.primary[95]};
    background: url(${thumbOnBackground}) no-repeat center 4px / 33px 33px;

    transition: inset-inline-start 160ms linear;
  }

  &[data-active=false] .thumb {
    inset-inline-start: 0;

    background: url(${thumbOffBackground}) no-repeat center 2px / 35px 35px;
  }

  .outer-border {
    position: absolute;
    z-index: 0;
    inset: -3px;

    border-radius: 100px;
    background:
      linear-gradient(
        180deg,
        rgba(255, 255, 255, 0.00) 45.45%,
        rgba(255, 255, 255, 0.10) 100%),
      linear-gradient(
        180deg,
        rgba(0, 0, 0, 0.15) 0%,
        rgba(0, 0, 0, 0.00) 65.91%);

    filter: blur(0.5px);
  }

`
