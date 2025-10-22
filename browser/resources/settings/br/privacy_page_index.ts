/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  RegisterPolymerTemplateModifications,
  RegisterPolymerPrototypeModification,
  html
} from 'chrome://resources/brave/polymer_overriding.js'
import type { Route } from '../router.js';
import { routes } from '../route.js';
import { loadTimeData } from "../i18n_setup.js"
import { pageVisibility } from './page_visibility.js'
import '../brave_survey_panelist_page/brave_survey_panelist_page.js'
import '../site_settings/site_settings_autoplay.js'
import '../site_settings/site_settings_localhost.js'
import '../site_settings/site_settings_shields.js'
import '../site_settings/site_settings_brave_ai.js'
import '../site_settings/site_settings_solana.js'
import { ContentSettingsTypes } from '../site_settings/constants.js';

RegisterPolymerPrototypeModification({
  'settings-privacy-page-index': (prototype) => {
    const oldGetDefaultViews = prototype.getDefaultViews_;
    prototype.getDefaultViews_ = function () {
      const views = oldGetDefaultViews.call(this)
        // Hide the privacy guide promo view.
        .filter((v: string) => v !== 'privacyGuidePromo');

      // Add dataCollection view.
      views.splice(1, 0, 'dataCollection');

      // <if expr="enable_tor">
      // Add tor view if it should be shown.
      if (pageVisibility.braveTor) {
        views.splice(1, 0, 'tor');
      }
      // </if>

      return views;
    }

    const oldGetViewIdsForRoute = prototype.getViewIdsForRoute_;
    prototype.getViewIdsForRoute_ = function (route: Route) {
      if (route === routes.BRAVE_SURVEY_PANELIST) {
        return ['surveyPanelist'];
      }
      return oldGetViewIdsForRoute.call(this, route);
    }
  }
})

RegisterPolymerTemplateModifications({
  'settings-privacy-page-index': (templateContent) => {
    const viewManager = templateContent.querySelector('#viewManager')
    if (!viewManager) {
      throw new Error("Couldn't find a view manager for settings-privacy-page-index!")
    }

    // Hide the privacy guide promo
    const privacyGuidePromoTemplate = templateContent.
      querySelector('template[is=dom-if][if="[[isPrivacyGuideAvailable]]"]')
    if (privacyGuidePromoTemplate) {
      privacyGuidePromoTemplate.remove()
    } else {
      throw new Error('[Settings] Missing privacyGuidePromoTemplate')
    }

    // <if expr="enable_tor">
    viewManager.appendChild(html`<template is="dom-if"
        if="[[showPage_(pageVisibility_.braveTor)]]">
      <settings-brave-tor-subpage
        id="tor"
        slot="view"
        prefs="{{prefs}}"
        in-search-mode="[[inSearchMode_]]">
      </settings-brave-tor-subpage>
    </template>`)
    // </if>

    viewManager.appendChild(html`<settings-brave-data-collection-subpage
      id="dataCollection"
      slot="view"
      prefs="{{prefs}}"
      in-search-mode="[[inSearchMode_]]">
    </settings-brave-data-collection-subpage>`)

    if (loadTimeData.getBoolean('isSurveyPanelistAllowed')) {
      viewManager.appendChild(html`<settings-brave-survey-panelist-page
        id="surveyPanelist"
        data-parent-view-id="dataCollection"
        slot="view"
        prefs="{{prefs}}"
        in-search-mode="[[inSearchMode_]]">
      </settings-brave-survey-panelist-page>`)
    }

    viewManager.appendChild(html`
      <site-settings-shields-page
          id="${ContentSettingsTypes.BRAVE_SHIELDS}"
          route-path$="[[routes_.SITE_SETTINGS_SHIELDS_STATUS.path]]"
          data-parent-view-id="siteSettings"
          slot="view"
          in-search-mode="[[inSearchMode_]]">
        </site-settings-shields-page>`)

    viewManager.appendChild(html`
      <site-settings-autoplay-page
          id="${ContentSettingsTypes.AUTOPLAY}"
          route-path$="[[routes_.SITE_SETTINGS_AUTOPLAY.path]]"
          data-parent-view-id="siteSettings"
          slot="view"
          in-search-mode="[[inSearchMode_]]">
        </site-settings-autoplay-page>`)

    if (loadTimeData.getBoolean('isLocalhostAccessFeatureEnabled')) {
      viewManager.appendChild(html`
            <site-settings-localhost-page
                id="${ContentSettingsTypes.LOCALHOST_ACCESS}"
                route-path$="[[routes_.SITE_SETTINGS_LOCALHOST_ACCESS.path]]"
                data-parent-view-id="siteSettings"
                slot="view"
                in-search-mode="[[inSearchMode_]]">
              </site-settings-localhost-page>`)
    }

    if (loadTimeData.getBoolean('isOpenAIChatFromBraveSearchEnabled')) {
      viewManager.appendChild(html`
      <site-settings-brave-ai-page
          id=${ContentSettingsTypes.BRAVE_OPEN_AI_CHAT}
          route-path$="[[routes_.SITE_SETTINGS_BRAVE_OPEN_AI_CHAT.path]]"
          data-parent-view-id="siteSettings"
          slot="view"
          in-search-mode="[[inSearchMode_]]">
        </site-settings-brave-ai-page>`)

      if (loadTimeData.getBoolean('isBraveWalletAllowed') && loadTimeData.getBoolean('isNativeBraveWalletFeatureEnabled')) {
      viewManager.appendChild(html`
      <site-settings-solana-page
          id="${ContentSettingsTypes.SOLANA}"
          route-path$="[[routes_.SITE_SETTINGS_SOLANA.path]]"
          data-parent-view-id="siteSettings"
          slot="view"
          in-search-mode="[[inSearchMode_]]">
        </site-settings-solana-page>`)
    }

    // Move the safety hub to the end of the page
    const safetyHubTemplate = templateContent.querySelector(
      'template[is=dom-if][if="[[showPage_(pageVisibility_.safetyHub)]]"]')
    if (safetyHubTemplate && safetyHubTemplate.parentElement) {
      safetyHubTemplate.parentElement.appendChild(safetyHubTemplate)
    } else {
      throw new Error('[Settings] Missing safetyHubTemplate')
    }
  }
})
