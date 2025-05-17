// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/md_select.css.js'
import 'chrome://resources/brave/leo.bundle.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { WebUiListenerMixin } from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { loadTimeData } from '../i18n_setup.js'
import { getTemplate } from './brave_survey_panelist_page.html.js'

const BraveSurveyPanelistPageBaseElement =
  WebUiListenerMixin(I18nMixin(PrefsMixin(PolymerElement)))

/**
 * 'settings-brave-survey-panalist-page' is the settings page containing
 * brave's Survey Panelist features.
 */
class BraveSurveyPanelistPageElement extends BraveSurveyPanelistPageBaseElement {
  static get is() {
    return 'settings-brave-survey-panelist-page'
  }

  static get template() {
    return getTemplate()
  }

  openLearnMore_() {
    window.open(loadTimeData.getString('braveSurveyPanelistLearnMoreURL'), "_blank", "noopener noreferrer")
  }
}

customElements.define(
  BraveSurveyPanelistPageElement.is, BraveSurveyPanelistPageElement)
