/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'
import { color, font } from '@brave/leo/tokens/css/variables'

export const style = scoped.css`
  & {
    background: ${color.container.background};
  }
`

style.passthrough.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
  }

  p {
    margin: 0;
  }

  h1 {
    font: ${font.heading.h1};
    margin: 0;
  }

  h2 {
    font: ${font.heading.h2};
    margin: 0;
  }

  h3 {
    font: ${font.heading.h3};
    margin: 0;
  }

  h4 {
    font: ${font.heading.h4};
    margin: 0;
  }

  button {
    margin: 0;
    padding: 0;
    background: 0;
    border: none;
    text-align: unset;
    width: unset;
    font: inherit;
    cursor: pointer;

    &:disabled {
      cursor: default;
    }
  }

  main {
    display: flex;
    flex-direction: column;
    gap: 8px;
    max-height: calc(var(--app-screen-height, 9999px) - 64px);

    > .scrollable {
      overflow-y: auto;
      flex: 1 1 auto;
      margin-top: -8px;
      margin-bottom: -8px;
    }
  }
`
