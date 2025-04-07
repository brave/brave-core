/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    padding: 16px;
    display: flex;
    justify-content: center;
    gap: 32px;
    min-height: 100vh;
    color: ${color.text.primary};
  }

  .update-available {
    --leo-button-color: rgba(255, 255, 255, 0.1);

    position: fixed;
    inset-block-start: 32px;

    leo-button {
      border-radius: 20px;
      overflow: hidden;
      backdrop-filter: brightness(0.8) blur(32px);
    }
  }

  .loading {
    --leo-progressring-color: rgba(255, 255, 255, 0.25);
    --leo-progressring-size: 32px;

    display: flex;
    justify-content: center;

    > * {
      margin: 16px 0;
    }
  }

  .sidebar {
    flex: 0 0 270px;
  }

  .feed-items {
    flex: 0 0 540px;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .feed-card {
    background: rgba(255, 255, 255, 0.05);
    border-radius: 16px;
    padding: 12px 16px 16px;
  }

  .caught-up {
    --leo-icon-size: 24px;

    display: flex;
    align-items: center;
    gap: 16px;
    color: rgba(255, 255, 255, 0.5);
    font: ${font.small.regular};
    padding: 16px 0;

    hr {
      flex: 1 1 auto;
      border-color: rgba(255, 255, 255, 0.1);
    }

    p {
      display: flex;
      align-items: center;
      gap: 6px;
    }
  }

  .controls-container {
    flex: 0 0 270px;
  }

  .controls {
    --leo-button-color: rgba(255, 255, 255, 0.5);
    --leo-button-radius: 4px;
    --leo-button-padding: 4px;

    position: fixed;
    inset-block-end: 48px;
    inset-inline-end: 48px;
    border-radius: 8px;
    padding: 8px;
    background: rgba(255, 255, 255, 0.05);
    backdrop-filter: blur(64px);
    display: flex;
    gap: 8px;
  }
`
