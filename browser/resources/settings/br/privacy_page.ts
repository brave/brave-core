/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {html, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {getTrustedHTML} from 'chrome://resources/js/static_types.js'
import {loadTimeData} from '../i18n_setup.js'

function InsertGoogleSignInSubpage (section: Element)
{
  section.appendChild(
    html`
      <template is="dom-if" route-path="/content/googleSignIn" no-search>
        <settings-subpage
          associated-control="[[$$('#googleSignIn')]]"
          page-title="${loadTimeData.getString('siteSettingsGoogleSignIn')}">
          <settings-category-default-radio-group
              id="googleSignInDefault"
              category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]"
              block-option-label=
                "${loadTimeData.getString('siteSettingsGoogleSignInBlock')}"
              allow-option-label=
                "${loadTimeData.getString('siteSettingsGoogleSignInAsk')}"
              allow-option-icon="user"
              block-option-icon="user-off">
          </settings-category-default-radio-group>
          <category-setting-exceptions
            id="googleSignInExceptions"
            category="[[contentSettingsTypesEnum_.GOOGLE_SIGN_IN]]"
            block-header=
              "${loadTimeData.getString(
                'siteSettingsGoogleSignInBlockExceptions')}"
            allow-header=
              "${loadTimeData.getString(
                'siteSettingsGoogleSignInAllowExceptions')}">
          </category-setting-exceptions>
        </settings-subpage>
      </template>
    `)
}

RegisterPolymerTemplateModifications({
  'settings-privacy-page': (templateContent) => {
    const section = templateContent.querySelector('settings-section')
    if (!section) {
      console.error(
        `[Settings] Couldn't find privacy_page settings-section`)
      return
    }
    const isGoogleSignInFeatureEnabled =
      loadTimeData.getBoolean('isGoogleSignInFeatureEnabled')
    if (isGoogleSignInFeatureEnabled) {
      InsertGoogleSignInSubpage(section)
    }

    const permissionsLinkRow =
      templateContent.getElementById('permissionsLinkRow')
    if (!permissionsLinkRow) {
      console.error(
        '[Brave Settings Overrides] Couldn\'t find permissionsLinkRow')
    } else {
      permissionsLinkRow.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`
          <settings-brave-personalization-options prefs="{{prefs}}">
          </settings-brave-personalization-options>
        `)
    }
    const thirdPartyCookiesLinkRow =
      templateContent.getElementById('thirdPartyCookiesLinkRow')
    if (!thirdPartyCookiesLinkRow) {
      console.error(
        '[Brave Settings Overrides] Could not find ' +
        'thirdPartyCookiesLinkRow id on privacy page.')
    } else {
      thirdPartyCookiesLinkRow.setAttribute('hidden', 'true')
    }

    if (!loadTimeData.getBoolean('isPrivacySandboxRestricted')) {
      const privacySandboxSettings3Template = templateContent.
        querySelector(`template[if*='isPrivacySandboxSettings3Enabled_']`)
      if (!privacySandboxSettings3Template) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if*=isPrivacySandboxSettings3Enabled_ on privacy page.')
      } else {
        const privacySandboxLinkRow = privacySandboxSettings3Template.content.
          getElementById('privacySandboxLinkRow')
        if (!privacySandboxLinkRow) {
          console.error(
            '[Brave Settings Overrides] Could not find privacySandboxLinkRow' +
            ' id on privacy page.')
        } else {
          privacySandboxLinkRow.setAttribute('hidden', 'true')
        }
        const privacySandboxLink = privacySandboxSettings3Template.content.
          getElementById('privacySandboxLink')
        if (!privacySandboxLink) {
          console.error(
            '[Brave Settings Overrides] Could not find privacySandboxLink id' +
            ' on privacy page.')
        } else {
          privacySandboxSettings3Template.setAttribute('hidden', 'true')
        }
      }
      const privacySandboxSettings4Template = templateContent.
        querySelector(`template[if*='isPrivacySandboxSettings4Enabled_']`)
      if (!privacySandboxSettings4Template) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if*=isPrivacySandboxSettings4Enabled_ on privacy page.')
      } else {
        const privacySandboxLinkRow = privacySandboxSettings4Template.content.
          getElementById('privacySandboxLinkRow')
        if (!privacySandboxLinkRow) {
          console.error(
            '[Brave Settings Overrides] Could not find privacySandboxLinkRow ' +
            'id on privacy page.')
        } else {
          privacySandboxLinkRow.setAttribute('hidden', 'true')
        }
      }
    }

    const showPrivacyGuideEntryPointTemplate =
      templateContent.querySelector(`template[if*='isPrivacyGuideAvailable']`)
    if (!showPrivacyGuideEntryPointTemplate) {
      console.error(
        '[Brave Settings Overrides] Could not find template with' +
        ' if*=isPrivacyGuideAvailable on privacy page.')
    } else {
      const privacyGuideLinkRow = showPrivacyGuideEntryPointTemplate.content.
        getElementById('privacyGuideLinkRow')
      if (!privacyGuideLinkRow) {
        console.error(
          '[Brave Settings Overrides] Could not find privacyGuideLinkRow id' +
          ' on privacy page.')
      } else {
        privacyGuideLinkRow.setAttribute('hidden', 'true')
      }
    }
  }
})
