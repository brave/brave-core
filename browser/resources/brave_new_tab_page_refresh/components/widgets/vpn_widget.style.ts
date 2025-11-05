/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, gradient } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .title {
    --leo-icon-size: 16px;
    --leo-icon-color: ${gradient.iconsActive};

    font: ${font.components.buttonSmall};
    display: flex;
    gap: 16px;
    align-items: center;

    > * {
      display: flex;
      gap: 8px;
      align-items: center;
    }
  }

  .provider {
    opacity: 0.3;
    color: #fff;
    font: ${font.xSmall.regular};
    display: flex;
    align-items: center;
    gap: 4px;

    img {
      height: 12px;
      width: auto;
    }
  }

  .content {
    display: flex;
    gap: 12px;
  }

  .features {
    --leo-icon-size: 12px;
    --leo-icon-color: ${color.icon.disabled};

    flex: 1 1 auto;
    font: ${font.xSmall.regular};
    display: flex;
    flex-direction: column;
    gap: 4px;

    > * {
      display: flex;
      align-items: center;
      gap: 8px;
    }
  }

  .purchase-actions {
    display: flex;
    flex-direction: column;
    gap: 4px;
    align-items: center;

    leo-button {
      --leo-button-color: rgba(255, 255, 255, 0.20);
    }

    .restore {
      opacity: 0.5;
      color: #fff;
      font: ${font.xSmall.regular};
      text-decoration: underline;
    }
  }

  .connection-graphic {
    min-width: 62px;

    img {
      height: 62px;
      width: auto;
    }
  }

  .status {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    margin-bottom: 4px;
  }

  .connected .status {
    color: #5FDA5C;
  }

  .connection-info {
    flex: 1 1 auto;
  }

  .region {
    font: ${font.xSmall.regular};
    color: ${color.text.secondary};
  }

  .country {
    display: flex;
    align-items: center;
    gap: 8px;
    font: ${font.large.semibold};
    color: ${color.text.primary};

    button {
      font: ${font.xSmall.link};
      text-decoration: underline;
    }
  }

  .connection-toggle {
    margin-top: 16px;
  }

`
