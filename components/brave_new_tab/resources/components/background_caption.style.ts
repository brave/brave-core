/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  a {
    text-decoration: none;
    color: inherit;
  }

  .photo-credits {
    color: ${color.white};
    font: ${font.xSmall.regular};
    text-shadow: 0 1px 0 rgba(255, 255, 255, 0.10);
    opacity: .5;
  }

  .sponsored-logo {
    --leo-icon-size: 20px;

    display: flex;
    flex-direction: column;
    align-items: end;
    color: ${color.white};

    leo-icon {
      opacity: 0;
      transition: opacity 200ms;
    }

    img {
      margin: 2px 20px 0 20px;
      width: 170px;
      height: auto;
    }

    &:hover {
      leo-icon {
        opacity: .7;
      }
    }
  }
`
