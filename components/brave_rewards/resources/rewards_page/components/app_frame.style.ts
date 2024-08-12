/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'

import { scopedCSS, css } from '../lib/scoped_css'

export const style = scopedCSS('app-frame', css`
  & {
    --leo-icon-color: ${color.icon.default};
  }

  &.panel-frame {
    block-size: 100cqb;
    display: flex;
    flex-direction: column;

    header {
      padding: 12px 16px;
      display: flex;
      align-items: center;
      background: transparent;
      transition: all 400ms;
      border-bottom: solid 1px transparent;

      &.overlapped {
        background:
          color-mix(in srgb, ${color.container.background} 60% , transparent);
        border-bottom-color: ${color.divider.subtle};
        box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);
      }
    }

    h4 {
      flex: 1 0 auto;
      font: ${font.heading.h4};
      color: ${color.text.primary};
      margin: 0;
      text-align: center;
    }

    leo-buttonmenu {
      height: 24px;
    }

    button {
      --leo-icon-size: 24px;
    }

    main {
      flex: 1 1 auto;
      overflow: auto;
      scrollbar-width: none;
      padding: 0 16px 16px;
    }

    footer {
      flex: 0 0 auto;
      background: ${color.container.background};
      padding: 8px;
      box-shadow: 0px -8px 16px rgba(0, 0, 0, 0.04);
    }

    ul {
      display: flex;
      align-content: stretch;
      gap: 4px;
    }

    li {
      position: relative;
      flex: 1 0 auto;

      a {
        --leo-icon-size: 24px;

        border-radius: 8px;
        color: ${color.text.secondary};
        font: ${font.small.regular};
        text-decoration: none;
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 2px;
        padding: 2px 0;
      }

      a.current {
        background: ${color.container.highlight};
      }
    }

    .expand-button {
      visibility: hidden;

      .is-bubble & {
        visibility: visible;
      }
    }
  }

  &.page-frame {
    display: flex;

    .sidebar {
      min-block-size: 100cqb;
      flex: 0 1 250px;
      display: flex;
      flex-direction: column;
      position: sticky;
      background: ${color.container.background};
    }

    header {
      padding: 40px 24px 16px;
    }

    nav {
      flex: 1 0 auto;
      overflow: auto;
    }

    .page-content {
      flex: 1 0 auto;
      margin: 16px;
    }

    main {
      margin: 0 auto;
      max-width: 768px;
    }

    footer {
      padding-bottom: 24px;
    }

    ul {
      list-style-type: none;
      padding: 0;
      margin: 0;
    }

    li {
      position: relative;

      a, button {
        --leo-icon-size: 18px;

        width: 100%;
        color: ${color.text.secondary};
        font: ${font.components.navbutton};
        text-decoration: none;
        display: flex;
        align-items: center;
        gap: 16px;
        padding: 13px 24px;

        &:hover {
          background: ${color.container.highlight};
        }
      }

      a.current {
        --leo-icon-color: ${color.icon.interactive};
        color: ${color.text.interactive};

        &::before {
          content: '';
          position: absolute;
          inset-block-start: 8px;
          inset-inline-start: 0;
          display: block;
          inline-size: 4px;
          block-size: 32px;
          background: ${color.icon.interactive};
          border-start-end-radius: 2px;
          border-end-end-radius: 2px;
        }
      }
    }
  }

  .more-menu {
    --leo-icon-size: 20px;
    --leo-menu-control-width: 212px;

    font: ${font.default.regular};
    display: block;

    leo-menu-item {
      color: ${color.text.primary};
      display: flex;
      padding: 8px;
      gap: 16px;
      align-items: center;
      min-width: 212px;

      &.reset {
        color: ${color.systemfeedback.errorText};
      }
    }
  }
`)
