/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect, font } from '@brave/leo/tokens/css/variables'

import { css, scopedCSS } from '../../lib/scoped_css'

export const style = scopedCSS('payout-account-card', css`
  h4 {
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.systemfeedback.successIcon};

    padding: 16px;
    display: flex;
    gap: 8px;
    align-items: center;

    .title {
      flex: 1 0 auto;
    }

    .info {
      --leo-icon-size: 20px;
      --leo-icon-color: ${color.icon.default};
    }

    leo-tooltip [slot=content] {
      max-width: 200px;
    }

    svg {
      height: 24px;
      width: auto;
      display: block;
    }
  }

  section {
    display: flex;
    justify-content: space-between;
    gap: 16px;
    padding: 16px;
  }

  .reconnect {
    --provider-icon-color: currentcolor;

    align-items: center;

    .text {
      flex: 1 1 auto;
    }

    svg {
      height: 20px;
      width: auto;
      display: block;
    }

    leo-button {
      white-space: nowrap;
    }
  }

  label {
    font: ${font.small.regular};
    opacity: 0.6;
  }

  .balance {
    display: flex;
    flex-direction: column;
    gap: 4px;
    font: ${font.heading.h3};
  }

  .account {
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 4px;
    font: ${font.default.semibold};
  }

  .account-drop-down {
    display: flex;
    gap: 4px;
    align-items: center;
    flex-wrap: nowrap;
    border-radius: 8px;
    padding: 2px 0 2px 4px;

    &:hover, &.open {
      background: ${color.elevation.primary};
    }

    .provider-icon svg {
      display: block;
      height: 16px;
      width: auto;
    }
  }

  .account-details {
    position: absolute;
    z-index: 1;
    inset-block-start: calc(100% + 2px);
    inset-inline-end: 0;
    border: 1px solid ${color.divider.subtle};
    border-radius: 8px;
    background: ${color.page.background};
    box-shadow: ${effect.elevation['03']};
    padding: 6px;
    display: flex;
    flex-direction: column;
    gap: 4px;
    cursor: default;

    .header {
      --self-provider-gap: 8px;
      --self-provider-icon-width: 20px;

      padding: 12px;
      display: flex;
      flex-direction: column;
      gap: 4px;
      font: ${font.default.semibold};
    }

    .provider {
      display: flex;
      align-items: center;
      gap: var(--self-provider-gap);
      white-space: nowrap;

      svg {
        height: auto;
        width: var(--self-provider-icon-width);
        display: block;
      }
    }

    .provider-name {
      flex: 1 0 auto;
    }

    .account-name {
      font: ${font.xSmall.regular};
      color: ${color.text.tertiary};
      padding-left:
        calc(var(--self-provider-icon-width) + var(--self-provider-gap));
    }

    button {
      --leo-icon-size: 20px;

      display: flex;
      gap: 16px;
      width: 100%;
      padding: 8px;
      background: ${color.container.background};
      border-radius: 4px;
      white-space: nowrap;
      font: ${font.default.regular};

      &:hover {
        --leo-icon-color: ${color.text.interactive};
      }
    }
  }
`)
