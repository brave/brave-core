/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import logoImage from './brave_logo.svg'

export const style = scoped.css`
  .sidebar {
    position: fixed;
    inset: 0 auto 0 0;
    background: ${color.page.background};
    width: 360px;
    height: unset;
    z-index: 10;
    box-shadow: ${effect.elevation['03']};
    border-radius: 0 16px 16px 0;
    color: ${color.text.primary};
    transform: translateX(-360px);

    transition:
      opacity 200ms,
      transform 200ms,
      display 200ms allow-discrete,
      overlay 200ms allow-discrete;

    &:popover-open {
      transform: translateX(0);
      opacity: 1;

      @starting-style {
        transform: translateX(-360px);
        opacity: 0;
      }
    }
  }

  .header {
    padding: 16px 20px;
    display: flex;
    align-items: center;
    gap: 23px;
    border-bottom: solid 1px ${color.divider.subtle};

    button {
      --leo-icon-size: 20px;
      --leo-icon-color: ${color.icon.default};
    }

    .logo {
      flex: 1 1 auto;
      height: 28px;
      background-image: url(${logoImage});
      background-repeat: no-repeat;
      background-size: auto 28px;
    }
  }

  .filter {
    padding: 8px 8px 8px 2px;
    display: flex;
    align-items: center;
    gap: 8px;

    .input-container {
      --leo-icon-size: 20px;

      flex: 1 1 auto;
      padding: 11px 8px;
      display: flex;
      align-items: center;
      gap: 8px;
      border: solid 1px ${color.divider.subtle};
      border-radius: 8px;
    }

    input {
      border: none;
      padding: 0;
      margin: 0;
      background: none;
      font: ${font.default.regular};
      flex: 1 1 auto;
      outline: none;
    }

    button {
      --leo-icon-size: 20px;

      padding: 11px;
      border: solid 1px ${color.divider.subtle};
      border-radius: 50%;
    }
  }

  .history-label {
    --leo-icon-size: 16px;

    padding: 10px;
    display: flex;
    align-items: center;
    gap: 8px;
    color: ${color.text.tertiary};
    font: ${font.components.label};
  }

  .conversation-list {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 8px 8px 8px 2px;

    a {
      padding: 8px 12px;
      border-radius: 8px;
      color: ${color.text.secondary};
      text-decoration: none;
      text-overflow: ellipsis;
      overflow: hidden;
      white-space: nowrap;

      &:hover {
        background: ${color.container.background};
      }
    }
  }
`
