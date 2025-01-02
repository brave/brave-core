// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterPolymerComponentBehaviors, RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

// TODO: move throttle utility to a chrome://resources module
function throttle (callback, maxWaitTime = 30) {
  // Call on first invocation
  let shouldWait = false;
  return function (...args) {
    if (!shouldWait) {
      callback.apply(this, args);
      shouldWait = true;
      setTimeout(function () {
        shouldWait = false;
      }, maxWaitTime);
    }
  }
}

RegisterStyleOverride(
  'settings-ui',
  html`
    <style>
      :host {
        display: inline-block;
        /* we will enforce sizing at the container level and let children fill 100% of parent
          top-down, rather than chromium's strategy of children setting their max width
          bottom-up, which can be confusing, especially when our layout is different. */
        --cr-centered-card-container_-_max-width: 100% !important; /* was 680px */
        --brave-settings-content-max-width: 708px;
        --brave-settings-menu-width: var(--settings-menu-width);
        --brave-settings-menu-margin: 12px;
        background-color: var(--leo-color-container-background);
      }

      /** Styling tweaks for the settings-menu when we display it inside the
        * drawer. */
      cr-drawer settings-menu {
        --brave-settings-menu-margin-v: 0;
        --brave-settings-menu-padding: 24px;
        --brave-settings-menu-margin: 0;
      }

      cr-drawer settings-menu::part(header) {
        display: none;
      }
      .cr-container-shadow {
        display: none !important;
     }
      #container {
        /* menu and content next to each other in the horizontal center */
      }
      #left {
        max-width: 250px;
      }
      #main {
        margin: var(--leo-spacing-m) var(--leo-spacing-m) var(--leo-spacing-m) 0;
        height: calc(100% - 40px);
        background: var(--leo-color-page-background);
        padding-bottom: var(--leo-spacing-2xl);
        border-radius: var(--leo-radius-xl) var(--leo-radius-m) var(--leo-radius-m) var(--leo-radius-xl);
        overflow: auto;
      }
      #right {
        /* this element is only a space filler in chromium */
        display: none;
      }
      @media (prefers-color-scheme: dark) {
        #container {
        }
      }
      @media (max-width: 980px) {
        #main {
          margin: 0;
          border-radius: 0;
          min-height: 100%;
        }
      }
    </style>
  `
)

const BraveClearSettingsMenuHighlightBehavior = {
  ready: function() {
    // Clear menu selection after scrolling away.
    // Chromium's menu is not persistant, so does not have
    // this issue.
    const container = this.$.container
    if (!container) {
      console.error('Could not find #container in settings-ui module')
    }
    const menu = this.$$('settings-menu')
    if (!menu) {
      console.error('Could not find settings-menu in settings-ui module')
    }
    let onScroll
    function stopObservingScroll() {
      if (onScroll) {
        container.removeEventListener('scroll', onScroll)
        onScroll = null
      }
    }
    window.addEventListener('showing-section', ({ detail: section }) => {
      // Currently showing or about to scroll to `section`. If we're getting
      // further away from section top then section is no longer 'selected'.
      // TODO(petemill): If this wasn't a chromium module, we'd simply add a
      // handler for scrolling away, or have the menu change selection as we
      // scroll.
      stopObservingScroll()
      function calcDistance() {
        const sectionScrollTop = section.offsetTop
        const currentScrollTop = container.scrollTop
        return Math.abs(sectionScrollTop - currentScrollTop)
      }
      let distance = calcDistance()
      onScroll = throttle(() => {
        const latestDistance = calcDistance()
        if (latestDistance > distance) {
          menu.setSelectedUrl_('')
          stopObservingScroll()
        } else {
          distance = latestDistance
        }
      }, 100)
      container.addEventListener('scroll', onScroll)
    })
  }
}

RegisterPolymerComponentBehaviors({
  'settings-ui': [
    BraveClearSettingsMenuHighlightBehavior
  ]
})
