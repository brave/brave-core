/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'

export const completeStyles = `
  /* Privacy Cards Styles */
  .privacy-cards {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    width: 100%;
  }

  .privacy-card {
    display: flex;
    align-items: center;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']};
    background: ${color.material.regular};
    border-radius: ${radius.xl};
    cursor: pointer;
    transition: background-color 0.15s ease, filter 0.3s ease;
  }

  .privacy-card:hover {
    background: ${color.material.thick};
  }

  .privacy-card-icon {
    width: 120px;
    height: 100px;
    border-radius: ${radius.l};
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .privacy-card-content {
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
    flex: 1;
  }

  .privacy-card-title {
    font: ${font.heading.h3};
    color: ${color.text.primary};
    opacity: 0.9;
    margin: 0;
    transition: color 0.3s ease;
  }

  .privacy-card-subtitle {
    font: ${font.default.regular};
    color: ${color.text.secondary};
    margin: 0;
    transition: color 0.3s ease;
  }

  .privacy-card leo-checkbox {
    flex-shrink: 0;
  }

  .privacy-card-unchecked {
    filter: grayscale(1);
  }

  .privacy-card-unchecked .privacy-card-icon {
    opacity: 0.5;
  }

  .privacy-card-unchecked .privacy-card-title,
  .privacy-card-unchecked .privacy-card-subtitle {
    color: ${color.text.tertiary};
  }
`

