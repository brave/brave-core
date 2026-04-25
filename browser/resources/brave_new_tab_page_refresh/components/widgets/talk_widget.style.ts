/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import talkGraphic from '../../assets/talk_graphic.svg'

export const style = scoped.css`
  & {
    flex-grow: 1;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .title {
    font: ${font.components.buttonSmall};
  }

  .content {
    flex-grow: 1;
    display: flex;
    align-items: center;
  }

  .graphic {
    background-image: url(${talkGraphic});
    background-repeat: no-repeat;
    background-size: contain;
    background-position: center center;
    width: 56px;
    align-self: stretch;
  }

  .text {
    flex: 1 1 auto;
    display: flex;
    flex-direction: column;
    gap: 4px;
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    padding: 0 8px;
  }

  .header {
    font: ${font.default.semibold};
    color: ${color.text.primary};
  }

  .actions {
    padding: 0 8px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;

    leo-button {
      --leo-button-color: rgba(255, 255, 255, 0.20);
      white-space: nowrap;
    }

    a {
      opacity: 0.5;
      color: #fff;
      font: ${font.xSmall.regular};
      text-underline-position: under;
    }
  }
`
