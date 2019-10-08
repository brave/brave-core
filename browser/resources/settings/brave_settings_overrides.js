// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Local Scope
(function() {
// TODO: move to a module
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

// Utils
function createMenuElement (title, href, iconName) {
  const menuEl = document.createElement('a')
  menuEl.href = href
  menuEl.innerHTML = `
    <iron-icon icon="${iconName}"></iron-icon>
    ${title}
  `
  return menuEl
}

function getMenuElement (templateContent, href) {
  const menuEl = templateContent.querySelector(`a[href="${href}"]`)
  if (!menuEl) {
    console.error(`[Brave Settings Overrides] Could not find menu item '${href}'`)
  }
  return menuEl
}

function getSectionElement (templateContent, sectionName) {
  const sectionEl = templateContent.querySelector(`template[if="[[showPage_(pageVisibility.${sectionName})]]"]`) ||
    templateContent.querySelector(`settings-section[section="${sectionName}"]`)
  if (!sectionEl) {
    console.error(`[Brave Settings Overrides] Could not find section '${sectionName}'`)
  }
  return sectionEl
}

if (!BravePatching) {
  console.error('BravePatching was not available to brave_settings_overrides.js')
}

//
// Override, extend or modify existing modules
//

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
      // Currently showing or about to scroll to `section`.
      // If we're getting further away from section top
      // then section is no longer 'selected'.
      // TODO(petemill): If this wasn't a chromium module, we'd simply add a handler
      // for scrolling away, or have the menu change selection as we scroll.
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

// Polymer Component Behavior injection (like superclasses)
BravePatching.RegisterPolymerComponentBehaviors({
  'settings-clear-browsing-data-dialog': [
    BraveClearBrowsingDataOnExitBehavior
  ],
  'settings-reset-profile-dialog': [
    BraveResetProfileDialogBehavior
  ],
  'settings-ui': [
    BraveClearSettingsMenuHighlightBehavior
  ]
})

// Templates
BravePatching.RegisterPolymerTemplateModifications({
  'settings-menu': (templateContent) => {
    // Add title
    const titleEl = document.createElement('h1')
    titleEl.id = 'settingsHeader'
    titleEl.innerHTML = loadTimeData.getString('settings')
    const topMenuEl = templateContent.querySelector('#topMenu')
    if (!topMenuEl) {
      console.error('[Brave Settings Overrides] Could not find topMenu element to add title after')
    } else {
      topMenuEl.insertAdjacentElement('afterbegin', titleEl)
    }
    // Advanced text
    const advancedToggle = templateContent.querySelector('#advancedButton span')
    if (!advancedToggle) {
      console.error('[Brave Settings Overrides] Could not find advancedButton to modify text')
    }
    advancedToggle.innerText = loadTimeData.getString('braveAdditionalSettingsTitle')
    // Add 'Get Started' item
    const peopleEl = getMenuElement(templateContent, '/people')
    const getStartedEl = createMenuElement(loadTimeData.getString('braveGetStartedTitle'), '/getStarted', 'brave_settings:get-started')
    peopleEl.insertAdjacentElement('afterend', getStartedEl)
    // Remove People item
    peopleEl.remove()
    // Move Appearance item
    const appearanceBrowserEl = getMenuElement(templateContent, '/appearance')
    getStartedEl.insertAdjacentElement('afterend', appearanceBrowserEl)
    // Add Sync item
    const syncEl = createMenuElement(loadTimeData.getString('braveSync'), '/braveSync', 'brave_settings:sync')
    appearanceBrowserEl.insertAdjacentElement('afterend', syncEl)
    // Add Shields item
    const shieldsEl = createMenuElement(loadTimeData.getString('braveShieldsTitle'), '/shields',  'brave_settings:shields')
    syncEl.insertAdjacentElement('afterend', shieldsEl)
    // Add Embed Blocking item
    const embedEl = createMenuElement(loadTimeData.getString('socialBlocking'), '/socialBlocking', 'brave_settings:social-permissions')
    shieldsEl.insertAdjacentElement('afterend', embedEl)
    // Move search item
    const searchEl = getMenuElement(templateContent, '/search')
    embedEl.insertAdjacentElement('afterend', searchEl)
    // Add Extensions item
    const extensionEl = createMenuElement(loadTimeData.getString('braveDefaultExtensions'), '/extensions', 'brave_settings:extensions')
    searchEl.insertAdjacentElement('afterend', extensionEl)
    // Remove default Browser
    const defaultBrowserEl = getMenuElement(templateContent, '/defaultBrowser')
    defaultBrowserEl.remove()
    // Remove Startup
    const startupEl = getMenuElement(templateContent, '/onStartup')
    startupEl.remove()
    // Remove Accessibility :-(
    const a11yEl = getMenuElement(templateContent, '/accessibility')
    a11yEl.remove()
    // Move autofill to advanced
    const autofillEl = getMenuElement(templateContent, '/autofill')
    const privacyEl = getMenuElement(templateContent, '/privacy')
    privacyEl.insertAdjacentElement('afterend', autofillEl)
    // Remove extensions link
    const extensionsLinkEl = templateContent.querySelector('#extensionsLink')
    if (!extensionsLinkEl) {
      console.error('[Brave Settings Overrides] Could not find extensionsLinkEl to remove')
    }
    extensionsLinkEl.remove()
    // Add version number to 'about' link
    const aboutEl = templateContent.querySelector('#about-menu')
    if (!aboutEl) {
      console.error('[Brave Settings Overrides] Could not find about-menun element')
    }
    const aboutTitleContent = aboutEl.innerHTML
    aboutEl.innerHTML = `
      <div class="brave-about-graphic">
        <iron-icon icon="brave_settings:full-color-brave-lion"><iron-icon>
      </div>
      <div class="brave-about-meta">
        <span class="brave-about-item brave-about-menu-link-text">${aboutTitleContent}</span>
        <span class="brave-about-item brave-about-menu-version">v ${loadTimeData.getString('braveProductVersion')}</span>
      </div>
    `
  },
  'settings-basic-page': (templateContent) => {
    // Routes
    const r = settings.router.routes_
    if (!r.BASIC) {
      console.error('[Brave Settings Overrides] Routes: could not find BASIC page')
    }
    r.GET_STARTED = r.BASIC.createSection('/getStarted', 'getStarted')
    // bring back people's /manageProfile (now in getStarted)
    r.MANAGE_PROFILE = r.GET_STARTED.createChild('/manageProfile');
    r.SHIELDS = r.BASIC.createSection('/shields', 'shields')
    r.SOCIAL_BLOCKING = r.BASIC.createSection('/socialBlocking', 'socialBlocking')
    r.EXTENSIONS = r.BASIC.createSection('/extensions', 'extensions')
    r.BRAVE_SYNC = r.BASIC.createSection('/braveSync', 'braveSync')
    if (!r.SITE_SETTINGS) {
      console.error('[Brave Settings Overrides] Routes: could not find SITE_SETTINGS page')
    }
    r.SITE_SETTINGS_AUTOPLAY = r.SITE_SETTINGS.createChild('autoplay')
    // Autofill route is moved to advanced,
    // otherwise its sections won't show up when opened.
    if (!r.AUTOFILL || !r.ADVANCED) {
      console.error('[Brave Settings Overrides] Could not move autofill route to advanced route', r)
    } else {
      r.AUTOFILL.parent = r.ADVANCED
    }
    // Add 'Getting Started' section
    // Entire content is wrapped in another conditional template
    const actualTemplate = templateContent.querySelector('template')
    if (!actualTemplate) {
      console.error('[Brave Settings Overrides] Could not find basic-page template')
      return
    }
    const basicPageEl = actualTemplate.content.querySelector('#basicPage')
    if (!basicPageEl) {
      console.error('[Brave Settings Overrides] Could not find basicPage element to insert Getting Started section')
    } else {
      const sectionGetStarted = document.createElement('template')
      sectionGetStarted.setAttribute('is', 'dom-if')
      sectionGetStarted.setAttribute('restamp', true)
      sectionGetStarted.setAttribute('if', '[[showPage_(pageVisibility.getStarted)]]')
      sectionGetStarted.innerHTML = `
        <settings-section page-title="${loadTimeData.getString('braveGetStartedTitle')}" section="getStarted">
          <brave-settings-getting-started prefs={{prefs}} page-visibility=[[pageVisibility]]></brave-settings-getting-started>
        </settings-section>
      `
      const sectionExtensions = document.createElement('template')
      sectionExtensions.setAttribute('is', 'dom-if')
      sectionExtensions.setAttribute('restamp', true)
      sectionExtensions.setAttribute('if', '[[showPage_(pageVisibility.extensions)]]')
      sectionExtensions.innerHTML = `
        <settings-section page-title="${loadTimeData.getString('braveDefaultExtensions')}" section="extensions">
          <settings-brave-default-extensions-page prefs="{{prefs}}"></settings-brave-default-extensions-page>
        </settings-section>
      `
      const sectionSync = document.createElement('template')
      sectionSync.setAttribute('is', 'dom-if')
      sectionSync.setAttribute('restamp', true)
      sectionSync.setAttribute('if', '[[showPage_(pageVisibility.braveSync)]]')
      sectionSync.innerHTML = `
        <settings-section page-title="${loadTimeData.getString('braveSync')}" section="braveSync">
          <settings-brave-sync-page prefs="{{prefs}}"></settings-brave-sync-page>
        </settings-section>
      `
      const sectionShields = document.createElement('template')
      sectionShields.setAttribute('is', 'dom-if')
      sectionShields.setAttribute('restamp', true)
      sectionShields.setAttribute('if', '[[showPage_(pageVisibility.shields)]]')
      sectionShields.innerHTML = `
        <settings-section page-title="${loadTimeData.getString('braveShieldsTitle')}"
            section="shields">
          <settings-default-brave-shields-page  prefs="{{prefs}}"></settings-default-brave-shields-page>
        </settings-section>
      `
      const sectionSocialBlocking = document.createElement('template')
      sectionSocialBlocking.setAttribute('is', 'dom-if')
      sectionSocialBlocking.setAttribute('restamp', true)
      sectionSocialBlocking.setAttribute('if', '[[showPage_(pageVisibility.socialBlocking)]]')
      sectionSocialBlocking.innerHTML = `
        <settings-section page-title="${loadTimeData.getString('socialBlocking')}"
            section="socialBlocking">
          <settings-social-blocking-page prefs="{{prefs}}"></settings-social-blocking-page>
        </settings-section>
      `
      // Get Started at top
      basicPageEl.insertAdjacentElement('afterbegin', sectionGetStarted)
      // Move Appearance item
      const sectionAppearance = getSectionElement(actualTemplate.content, 'appearance')
      sectionGetStarted.insertAdjacentElement('afterend', sectionAppearance)
      // Insert sync
      sectionAppearance.insertAdjacentElement('afterend', sectionSync)
      // Insert shields
      sectionSync.insertAdjacentElement('afterend', sectionShields)
      // Insert Social Blocking
      sectionShields.insertAdjacentElement('afterend', sectionSocialBlocking)
      // Move search
      const sectionSearch = getSectionElement(actualTemplate.content, 'search')
      sectionSocialBlocking.insertAdjacentElement('afterend', sectionSearch)
      // Insert extensions
      sectionSearch.insertAdjacentElement('afterend', sectionExtensions)
      // Remove 'startup'
      const sectionStartup = getSectionElement(actualTemplate.content, 'onStartup')
      sectionStartup.remove()
      // Advanced
      const advancedTemplate = templateContent.querySelector('template[if="[[showAdvancedSettings_(pageVisibility.advancedSettings)]]"]')
      if (!advancedTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced section')
      }
      const advancedSubSectionsTemplate = advancedTemplate.content.querySelector('settings-idle-load template')
      if (!advancedSubSectionsTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced sub-sections container')
      }
      const advancedToggleTemplate = advancedTemplate.content.querySelector('template')
      if (!advancedToggleTemplate) {
        console.error('[Brave Settings Overrides] Could not find advanced toggle template')
      }
      const advancedToggleText = advancedToggleTemplate.content.querySelector('cr-button span')
      if (!advancedToggleText) {
        console.error('[Brave Settings Overrides] Could not find advanced toggle text')
      }
      advancedToggleText.innerText = loadTimeData.getString('braveAdditionalSettingsTitle')
      // Move autofill to after privacy
      const sectionAutofill = getSectionElement(actualTemplate.content, 'autofill')
      const sectionPrivacy = getSectionElement(advancedSubSectionsTemplate.content, 'privacy')
      sectionPrivacy.insertAdjacentElement('afterend', sectionAutofill)
    }
  },
  'settings-default-browser-page': (templateContent) => {
    // has nested templates
    for (const templateEl of templateContent.querySelectorAll('template')) {
      for (const boxEl of templateEl.content.querySelectorAll('.settings-box')) {
        boxEl.classList.remove('first')
      }
    }
  },
  'settings-people-page': (templateContent) => {
    // People page needs to think it's in the getStarted section, since it is
    // (we remove the People section as a separate section).
    const page = templateContent.querySelector('settings-animated-pages[section=people]')
    page.setAttribute('section', 'getStarted')
    const profileTemplate = templateContent.querySelector('template[is="dom-if"][if="[[diceEnabled_]]"]')
    // In chromium, the whole 'manage profile' section is only
    // displayed if dice is enabled.
    // Instead, always show it, but remove the google account specific entry.
    profileTemplate.setAttribute('if', 'true')
    const manageGoogleAccount = profileTemplate.content.querySelector('#manage-google-account')
    if (!manageGoogleAccount) {
      console.error('[Brave Settings Overrides] Could not find the google account settings item')
    }
    manageGoogleAccount.remove()
    // Edit profile item needs to know it's the first in the section
    const firstItem = profileTemplate.content.querySelector('#edit-profile')
    if (!firstItem) {
      console.error('[Brave Settings Overrides] Could not find #edit-profile item in people_page')
      return
    }
    firstItem.classList.add('first')
  },
  'settings-payments-section': (templateContent) => {
    const manageLink = templateContent.querySelector('#manageLink')
    if (!manageLink) {
      console.error('[Brave Settings Overrides] Could not find manage payments link')
    }
    manageLink.remove()
  }
})

// Icons
BravePatching.OverrideIronIcons('settings', 'brave_settings', {
  palette: 'appearance',
  assignment: 'autofill',
  language: 'language',
  build: 'system',
  restore: 'reset-settings'
})
BravePatching.OverrideIronIcons('cr', 'brave_settings', {
  security: 'privacy-security',
  search: 'search-engine',
  ['file-download']: 'download',
  print: 'printing'
})

//
// Register any new modules
//

Polymer({
  is: 'brave-settings-getting-started'
})

})() // execute local scope
