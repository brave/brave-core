/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'

export const layoutStyles = `
  .container {
    display: flex;
    flex-direction: column;
    flex-wrap: nowrap;
    max-width: 1130px;
    max-height: 700px;
    width: 100%;
    height: calc(100dvh - 2 * ${spacing.xl});
    background-color: ${color.material.thick};
    border-radius: ${radius.xxl};
    backdrop-filter: blur(35px);
    overflow: hidden;
  }

  .content-area {
    display: flex;
    flex: 1;
    position: relative;
    overflow: hidden;
  }

  /* Static Brave logo - stays in place during transitions */
  .content-area > .brave-logo-container {
    position: absolute;
    top: ${spacing['4Xl']};
    left: ${spacing['4Xl']};
    z-index: 10;
  }

  .content {
    display: flex;
    width: 100%;
    flex: 1;
    min-height: 0;
    max-height: 100%;
  }

  .left-content {
    max-width: 430px;
    width: 40%;
    padding: ${spacing['4Xl']};
    /* Add top padding to account for the absolutely positioned logo */
    padding-top: calc(${spacing['4Xl']} + 52px + ${spacing['4Xl']});
    max-height: 100%;
    overflow: auto;
  }

  .brave-logo-container {
    margin-bottom: ${spacing['4Xl']};
  }

  .brave-logo-container leo-icon {
    --leo-icon-size: 52px;
  }

  .left-text-content {
    display: flex;
    flex-direction: column;
  }

  .right-content {
    min-width: 700px;
    width: 60%;
    padding: ${spacing['4Xl']};
    display: flex;
    align-items: center;
    justify-content: center;
    max-height: 100%;
    overflow: auto;
  }

  /* Welcome page hero image */
  .right-content.welcome-hero {
    padding: 0;
    overflow: hidden;
    flex: 1;
    min-height: 0;
  }

  .hero-image {
    width: 100%;
    height: 100%;
    object-fit: contain;
    display: block;
  }

  .footer {
    display: flex;
    width: 100%;
    padding: ${spacing['2Xl']};
    align-items: center;
    flex-shrink: 0;
    background: linear-gradient(to bottom, ${color.material.thick}, transparent);
    justify-content: space-between;
  }

  .footer-left {
    display: flex;
    gap: ${spacing['2Xl']};
  }

  .footer-right {
    display: flex;
    gap: ${spacing['m']};
    text-align: right;
  }

  .main-button {
    min-width: 240px;
  }
`

