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
import {loadTimeData} from "../i18n_setup.js"
import {pageVisibility} from './page_visibility.js'

RegisterPolymerPrototypeModification({
  'settings-privacy-page-index': (prototype) => {
    const oldGetDefaultViews = prototype.getDefaultViews_;
    prototype.getDefaultViews_ = function () {
      const views = oldGetDefaultViews.call(this)
        // Hide the privacy guide promo view.
        .filter((v: string) => v !== 'privacyGuidePromo');

      // Add dataCollection view.
      views.splice(1, 0, 'dataCollection');

      // Add tor view if it should be shown.
      if (pageVisibility.braveTor) {
        views.splice(1, 0, 'tor');
      }

      return views;
    }

    // Add the subroute for the survey panelist page.
    const oldCurrentRouteChanged = prototype.currentRouteChanged;
    prototype.currentRouteChanged = function (newRoute: Route, oldRoute: Route) {
      if (newRoute === routes.BRAVE_SURVEY_PANELIST) {
        queueMicrotask(() => {
          this.$.viewManager.switchView('surveyPanelist', 'no-animation', 'no-animation');
        })
        return;
      }

      oldCurrentRouteChanged.call(this, newRoute, oldRoute);
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

    viewManager.appendChild(html`<template is="dom-if"
        if="[[showPage_(pageVisibility_.braveTor)]]">
      <settings-brave-tor-subpage
        id="tor"
        slot="view"
        prefs="{{prefs}}"
        in-search-mode="[[inSearchMode_]]">
      </settings-brave-tor-subpage>
    </template>`)

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
