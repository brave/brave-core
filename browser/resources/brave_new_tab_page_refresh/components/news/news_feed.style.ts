/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

const twoColumnBreakpoint = '1024px'
const oneColumnBreakpoint = '800px'

export const style = scoped.css`
  & {
    color-scheme: dark;
    container-type: inline-size;
    position: relative;
    padding: 16px 16px 48px;
    color: ${color.text.primary};
    min-height: 100vh;
    display: flex;
    flex-direction: column;
  }

  .opt-in-container {
    padding: 80px 0;
    max-width: 600px;
    margin: 0 auto;
  }

  .opt-in-card {
    background: rgba(255, 255, 255, 0.05);
    border-radius: 16px;
    padding: 32px;
    color: ${color.text.primary};
    text-align: center;
  }

  .news-feed {
    flex: 1 1 auto;
    display: flex;
    justify-content: center;
    gap: 32px;

    --news-feed-control-bar-height: 0px;

    @container (max-width: ${twoColumnBreakpoint}) {
      --news-feed-control-bar-height: 54px;
    }
  }

  .feed-list {
    flex: 0 1 540px;
  }

  .update-available {
    --leo-button-color: rgba(255, 255, 255, 0.1);

    position: fixed;
    z-index: 2;
    inset-block-start: calc(32px + var(--news-feed-control-bar-height));
    inset-inline-start: 0;
    inset-inline-end: 0;
    text-align: center;

    leo-button {
      border-radius: 20px;
      overflow: hidden;
      backdrop-filter: brightness(0.8) blur(32px);
    }
  }

  .sidebar {
    flex: 0 1 270px;

    @container (max-width: ${oneColumnBreakpoint}) {
      display: none;
    }

    > div {
      top: calc(16px + var(--news-feed-control-bar-height));
    }
  }

  .controls-container {
    flex: 0 1 270px;

    @container (max-width: ${twoColumnBreakpoint}) {
      position: fixed;
    }
  }

  .controls {
    --leo-button-color: rgba(255, 255, 255, 0.5);
    --leo-button-radius: 4px;
    --leo-button-padding: 4px;

    anchor-name: --ntp-news-controls;

    position: fixed;
    inset-block-end: 48px;
    inset-inline-end: 48px;
    border-radius: 8px;
    padding: 8px;
    background: rgba(255, 255, 255, 0.05);
    backdrop-filter: blur(64px);
    display: flex;
    align-items: center;
    gap: 8px;

    > * {
      flex: 0 0 auto;
    }

    .popover-nav-control {
      flex: 1 1 auto;
      display: none;

      @container (max-width: ${twoColumnBreakpoint}) {
        display: block;

        leo-button {
          display: none;
        }
      }

      @container (max-width: ${oneColumnBreakpoint}) {
        leo-button {
          display: block;
        }
      }
    }

    @container (max-width: ${twoColumnBreakpoint}) {
      inset-block-start: 0;
      inset-block-end: auto;
      inset-inline-start: 0;
      inset-inline-end: 0;
      border-radius: 0;
      padding: 12px 16px;
    }

    @container (max-width: ${oneColumnBreakpoint}) {
      .popover-nav-control {
        display: block;
      }
    }
  }

  .popover-nav {
    position: fixed;
    position-anchor: --ntp-news-controls;
    inset-block-start: anchor(bottom);
    inset-block-end: 0;
    inset-inline-start: 0;
    backdrop-filter: blur(64px);
    height: auto;
    width: 270px;

    transform: translateX(-270px);
    transition: all 200ms allow-discrete;

    &:popover-open {
      transform: translateX(0);

      @starting-style {
        transform: translateX(-270px);
      }
    }

    > div {
      background: none;
      position: static;
    }
  }

  .hidden-above-fold {
    animation: linear hidden-above-fold both;
    animation-timeline: scroll();
    animation-range: 90vh 110vh;
  }

  @keyframes hidden-above-fold {
    from {
      visibility: hidden;
    }
    to {
      visibility: visible;
    }
  }
`
