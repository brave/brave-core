/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

const tabbar = color.desktopbrowser.tabbar

// Browser chrome colors, provided by chrome://theme/colors.css. The Nala
// fallbacks apply where that stylesheet is unavailable (e.g. Storybook).
const browserColor = {
  toolbar: `var(--color-toolbar, ${color.desktopbrowser.chromeBackgroundDesktop})`,
  frame: `var(--color-frame-active, ${tabbar.background})`,
  activeTab: `var(--color-tab-background-active-frame-active, ${tabbar.activeTabHorizontal})`,
  tabTitle: `var(--color-tab-foreground-active-frame-active, ${color.text.primary})`,
  toolbarIcon: `var(--color-toolbar-button-icon, ${color.icon.default})`,
  locationBar: `var(--color-location-bar-background, ${color.desktopbrowser.omnibar.backgroundDesktop})`,
}

export const style = scoped.css`
  & {
    flex: 0 0 auto;
    height: 255px;
    padding: 32px;
    border-radius: ${radius.xl};
    background: ${color.material.regular};
    overflow: hidden;
  }

  .chrome {
    position: relative;
    height: 100%;
    display: flex;
    flex-direction: column;
    align-items: stretch;
    overflow: hidden;
    border-radius: ${radius.l};
    background: ${browserColor.toolbar};
    box-shadow: 0 0 0 0.75px rgba(6, 6, 5, 0.2);

    /* Highlights the top edge of the frame. Drawn as an overlay so that it
     * paints over the chrome's contents rather than behind them. */
    &::after {
      content: '';
      position: absolute;
      inset: 0;
      pointer-events: none;
      border-radius: inherit;
      box-shadow: inset 0 1px 0.5px rgba(255, 255, 255, 0.2);
    }
  }

  .top-bar {
    flex: 0 0 auto;
    display: flex;
    flex-direction: column;
    align-items: stretch;
  }

  .toolbar-button,
  .omnibox-button {
    --leo-icon-size: 20px;
    --leo-icon-color: ${browserColor.toolbarIcon};

    flex: 0 0 auto;
    display: flex;
    align-items: center;
    justify-content: center;
    min-width: 28px;
    height: 28px;
    padding: ${spacing.s};
    border-radius: ${radius.m};
  }

  .omnibox-button {
    border-radius: ${radius.s};
  }

  .toolbar-button.disabled {
    opacity: 0.5;
  }

  .address-bar {
    flex: 0 0 auto;
    display: flex;
    align-items: center;
    gap: ${spacing.xl};
    padding: ${spacing.s};
    background: ${browserColor.toolbar};
  }

  .nav-buttons {
    display: flex;
    gap: ${spacing.s};
  }

  .omnibox {
    flex: 1 1 auto;
    min-width: 0;
    display: flex;
    align-items: center;
    gap: ${spacing.m};
    padding: ${spacing.xs};
    border-radius: ${radius.m};
    background: ${browserColor.locationBar};
    overflow: hidden;

    .placeholder {
      flex: 1 1 auto;
      min-width: 0;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      font: ${font.default.regular};
      color: ${color.text.secondary};
    }
  }

  .active-tab {
    --leo-icon-size: 16px;
    --leo-icon-color: ${color.icon.default};

    display: flex;
    align-items: center;
    gap: ${spacing.m};
    height: 32px;
    padding: 0 ${spacing.m};
    border-radius: ${radius.m};
    overflow: hidden;

    .tab-title {
      flex: 1 1 auto;
      min-width: 0;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      font: ${font.default.regular};
      color: ${browserColor.tabTitle};
    }
  }

  .pinned-tab {
    --leo-icon-size: 16px;

    flex: 0 0 auto;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 32px;
    height: 32px;
    border: solid 1px transparent;
    border-radius: ${radius.m};
  }

  .chrome-body {
    flex: 1 1 auto;
    min-height: 0;
    display: flex;
    align-items: stretch;
  }

  .page {
    flex: 1 1 auto;
    min-width: 0;
    margin: 0 ${spacing.s} ${spacing.s};
    border-radius: ${radius.m};
    background: ${browserColor.locationBar};
  }

  &.horizontal {
    .top-bar {
      background: ${browserColor.frame};
    }

    .tab-strip {
      display: flex;
      align-items: center;
      gap: ${spacing.s};
      height: 40px;
      padding: 0 ${spacing.s};
    }

    /* The toolbar sits below the tab strip, so its top corners round away to
     * reveal the frame behind them. */
    .address-bar {
      border-radius: ${radius.m} ${radius.m} 0 0;
    }

    .pinned-tab {
      border-color: ${tabbar.pinnedTabOutlineHorizontal};
    }

    .active-tab {
      width: 228px;
      background: ${browserColor.activeTab};
    }

    .new-tab {
      margin-left: ${spacing.m};
    }

    .tab-strip-end {
      margin-left: auto;
    }
  }

  &.vertical {
    .tab-strip {
      flex: 0 0 auto;
      width: 244px;
      display: flex;
      flex-direction: column;
      overflow: hidden;
      background: ${browserColor.toolbar};
    }

    .pinned-tabs {
      display: flex;
      gap: ${spacing.s};
      padding: ${spacing.s};
    }

    .pinned-tab {
      flex: 1 1 0;
      width: auto;
      min-width: 0;
      border-color: ${tabbar.pinnedTabOutlineVertical};
    }

    .divider {
      height: 1px;
      background: ${color.desktopbrowser.toolbar.button.outline};
    }

    .tabs {
      flex: 1 1 auto;
      min-height: 0;
      display: flex;
      flex-direction: column;
      gap: ${spacing.s};
      padding: ${spacing.s};
    }

    .active-tab {
      background: ${tabbar.activeTabVertical};
    }

    .page {
      margin-left: 0;
    }
  }
`
