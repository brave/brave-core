// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '../brave_appearance_page/sidebar.js'
import '../brave_appearance_page/tabs.js'
import '../brave_content_page/content.js'
import '../brave_content_page/playlist.js'
import '../brave_content_page/speedreader.js'
import '../brave_data_collection_page/brave_data_collection_page.js'
import '../brave_default_extensions_page/brave_default_extensions_page.js'
import '../brave_new_tab_page/brave_new_tab_page.js'
import '../brave_search_engines_page/brave_search_engines_page.js'
import '../brave_sync_page/brave_sync_page.js'
import '../brave_tor_page/brave_tor_subpage.js'
import '../brave_wallet_page/brave_wallet_page.js'
import '../brave_web3_domains_page/brave_web3_domains_page.js'
import '../default_brave_shields_page/default_brave_shields_page.js'
import '../getting_started_page/getting_started.js'
import '../social_blocking_page/social_blocking_page.js'
import '../brave_leo_assistant_page/brave_leo_assistant_page.js'
import '../brave_leo_assistant_page/model_list_section.js'
import '../brave_leo_assistant_page/personalization.js'
import '../brave_survey_panelist_page/brave_survey_panelist_page.js'

// <if expr="enable_containers">
import '../brave_content_page/containers.js'
import { ContainersStrings } from '../brave_generated_resources_webui_strings.js'
// </if>

import {
  html,
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'

import {loadTimeData} from '../i18n_setup.js'

const isGuest = loadTimeData.getBoolean('isGuest')

export function getSectionElement (
  templateContent: HTMLTemplateElement,
  sectionName: string)
{
  const sectionEl = templateContent.querySelector(`template[if*='showPage_(pageVisibility_.${sectionName}']`) ||
    templateContent.querySelector(`template[if*='pageVisibility_.${sectionName}']`) ||
    templateContent.querySelector(`settings-section[section="${sectionName}"]`)
  if (!sectionEl) {
    console.error(`[Brave Settings Overrides] Could not find section '${sectionName}'`)
  }
  return sectionEl
}

/**
 * Creates a settings-section element with a single child and returns it.
 * @param {string} sectionName - value of the section attribute
 * @param {string} titleName - loadTimeData key for page-title
 * @param {string} childName - name of child element
 * @param {Object} childAttributes - key-value pairs of child element attributes
 * @returns {Element}
 */
function createSectionElement (
  sectionName: string,
  titleName: string,
  childName: string,
  childAttributes: Record<string, string>)
{
  const childAttributesString = Object.keys(childAttributes).map(attribute =>
      `${attribute}="${childAttributes[attribute]}"`)
    .join(' ')
  // This needs to be inside a template so that our components do not get created immediately.
  // Otherwise the polymer bindings won't be setup correctly at first.
  return html`
    <settings-section page-title="${loadTimeData.getString(titleName)}" section="${sectionName}">
      <${childName}
        ${childAttributesString}
      >
      </${childName}>
    </settings-section>
  `
}

function createNestedSectionElement (
  sectionName: string,
  nestedUnder: string,
  titleName: string,
  childName: string,
  childAttributes: Record<string, string>)
{
  const childAttributesString = Object.keys(childAttributes).map(attribute =>
    `${attribute}="${childAttributes[attribute]}"`)
    .join(' ')
  // This needs to be inside a template so that our components do not get created immediately.
  // Otherwise the polymer bindings won't be setup correctly at first.
  return html`
    <settings-section id='${sectionName}-section' page-title="${loadTimeData.getString(titleName)}" section="${sectionName}" nest-under-section="${nestedUnder}">
      <${childName}
        ${childAttributesString}
      >
      </${childName}>
    </settings-section>
  `
}

RegisterStyleOverride(
  'settings-basic-page',
  html`
    <style>
      :host {
        min-width: 544px !important;
      }
    </style>
  ` as HTMLTemplateElement
)

RegisterPolymerTemplateModifications({
  'settings-basic-page': (templateContent) => {
    // Add 'Getting Started' section
    const basicPageEl = templateContent.querySelector('#basicPage')
    if (!basicPageEl) {
      throw new Error('[Settings] Missing basicPage element')
    } else {
      const sectionGetStarted = document.createElement('template')
      sectionGetStarted.setAttribute('is', 'dom-if')
      sectionGetStarted.setAttribute('restamp', 'true')
      sectionGetStarted.setAttribute('if', '[[showPage_(pageVisibility_.getStarted)]]')
      sectionGetStarted.content.appendChild(createSectionElement(
        'getStarted',
        'braveGetStartedTitle',
        'brave-settings-getting-started',
        {
          prefs: '{{prefs}}',
        }
      ))
      const sectionTabs = document.createElement('template')
      sectionTabs.setAttribute('is', 'dom-if')
      sectionTabs.setAttribute('restamp', 'true')
      sectionTabs.setAttribute('if', '[[showPage_(pageVisibility_.appearance)]]')
      sectionTabs.content.appendChild(createNestedSectionElement(
        'tabs',
        'appearance',
        'appearanceSettingsTabsSection',
        'settings-brave-appearance-tabs',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionSidebar = document.createElement('template')
      sectionSidebar.setAttribute('is', 'dom-if')
      sectionSidebar.setAttribute('restamp', 'true')
      sectionSidebar.setAttribute('if', '[[showPage_(pageVisibility_.appearance)]]')
      sectionSidebar.content.appendChild(createNestedSectionElement(
        'sidebar',
        'appearance',
        'sideBar',
        'settings-brave-appearance-sidebar',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionExtensions = document.createElement('template')
      sectionExtensions.setAttribute('is', 'dom-if')
      sectionExtensions.setAttribute('restamp', 'true')
      sectionExtensions.setAttribute('if', '[[showPage_(pageVisibility_.extensions)]]')
      sectionExtensions.content.appendChild(createSectionElement(
        'extensions',
        'braveDefaultExtensions',
        'settings-brave-default-extensions-page',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionTor = document.createElement('template')
      sectionTor.setAttribute('is', 'dom-if')
      sectionTor.setAttribute('restamp', 'true')
      sectionTor.setAttribute('if', '[[showPage_(pageVisibility_.braveTor)]]')
      sectionTor.content.appendChild(createNestedSectionElement(
        'tor',
        'privacy',
        'braveTor',
        'settings-brave-tor-subpage',
        {
          prefs: '{{prefs}}'
        }
      ))
      const sectionDataCollection = document.createElement('template')
      sectionDataCollection.setAttribute('is', 'dom-if')
      sectionDataCollection.setAttribute('restamp', 'true')
      sectionDataCollection.
        setAttribute('if', '[[showPage_(pageVisibility_.braveDataCollection)]]')
      sectionDataCollection.content.appendChild(createNestedSectionElement(
        'dataCollection',
        'privacy',
        'braveDataCollection',
        'settings-brave-data-collection-subpage',
        {
          prefs: '{{prefs}}'
        }
      ))

      const sectionSurveyPanelist = document.createElement('template')
      sectionSurveyPanelist.setAttribute('is', 'dom-if')
      sectionSurveyPanelist.setAttribute('restamp', 'true')
      sectionSurveyPanelist
        .setAttribute('if', '[[showPage_(pageVisibility_.surveyPanelist)]]')
      sectionSurveyPanelist.content.appendChild(createSectionElement(
        'surveyPanelist',
        'surveyPanelist',
        'settings-brave-survey-panelist-page',
        {
          prefs: '{{prefs}}'
        }
      ))

      // <if expr="enable_containers">
      const sectionContainers = document.createElement('template')
      sectionContainers.setAttribute('is', 'dom-if')
      sectionContainers.setAttribute('restamp', 'true')
      sectionContainers.setAttribute('if', '[[showPage_(pageVisibility_.containers)]]')
      sectionContainers.content.appendChild(createNestedSectionElement(
        'containers',
        'content',
        ContainersStrings.SETTINGS_CONTAINERS_SECTION_LABEL,
        'settings-brave-content-containers',
        {
          prefs: '{{prefs}}',
        }
      ))
      // </if>

      // Insert Tor
      let last = basicPageEl.insertAdjacentElement('afterend', sectionTor)
      // Insert Data collection
      last = last.insertAdjacentElement('afterend', sectionDataCollection)
      // Insert Surevy Panelist
      last = last.insertAdjacentElement('afterend', sectionSurveyPanelist)

      // <if expr="enable_containers">
      last = last.insertAdjacentElement('afterend', sectionContainers)
      // </if>
    }
  }
})
