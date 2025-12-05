/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'
import { color, effect, font, gradient } from '@brave/leo/tokens/css/variables'

import backgroundImage from '../assets/growing_shield.svg'

export const style = scoped.css`
  & {
    margin: 8px 8px 0;
    border-radius: 16px;
    border: solid 1px ${color.primitive.primary[25]};
    background:
      url(${backgroundImage}) no-repeat right top,
      ${gradient.primaryGradient};
    box-shadow: 0 3px 2px 0 ${color.primitive.primary[60]} inset;
    padding: 8px;
    color: #fff;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  &[data-shields-off='true'] {
    background: linear-gradient(
      180deg,
      ${color.neutral[10]} 0%,
      ${color.neutral[20]} 100%);
    box-shadow: none;
    border: none;
    color: ${color.text.secondary};
  }

  .site-info {
    padding: 24px 16px 16px;
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .site-text {
    flex: 1 1 auto;
    overflow: hidden;
  }

  h3 {
    text-overflow: ellipsis;
    white-space: nowrap;
    overflow: hidden;
  }

  .site-icon {
    padding: 6px;
    border-radius: 50%;
    background: ${color.container.background};
    box-shadow: ${effect.elevation['01']};

    img {
      display: block;
      width: 20px;
      height: 20px;
    }
  }

  .shields-status {
    font: ${font.large.regular};

    strong {
      font: ${font.large.semibold};
    }
  }

  .block-info {
    border-radius: 8px;
    padding: 12px 8px;
    background: linear-gradient(
      180deg,
      ${color.primitive.primary[50]} 0%,
      ${color.primitive.primary[35]} 100%);
    box-shadow:
      0 2px 5px 0 rgba(18, 28, 134, 0.50),
      0 1px 0 0 ${color.primitive.primary[60]} inset;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;

    .count {
      font: ${font.heading.h3};
    }
  }

  .report-prompt {
    padding: 8px 8px 8px 12px;
    border-radius: 8px;
    background: ${color.material.thin};
    display: flex;
    align-items: center;
    gap: 12px;

    leo-button {
      margin-left: auto;
      flex: 0 1 auto;
    }
  }
`
