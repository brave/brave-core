// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {loadTimeData} from '//resources/js/load_time_data.js'
import {RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'

RegisterPolymerTemplateModifications({
  'settings-security-page': (templateContent) => {
    const safeBrowsingReportingToggleSetting = templateContent.
      querySelector(
        'template[is=dom-if][if="[[!hideExtendedReportingRadioButton_]]"]'
    )
    if (!safeBrowsingReportingToggleSetting) {
      console.error(
        '[Brave Settings Overrides] Could not find template with ' +
        'if=[[!hideExtendedReportingRadioButton_]] on security page.'
      )
    } else {
      const safeBrowsingReportingToggle = safeBrowsingReportingToggleSetting.
        content.getElementById('safeBrowsingReportingToggle')
      if (!safeBrowsingReportingToggle) {
        console.error(
          '[Brave Settings Overrides] Could not find ' +
          ' safeBrowsingReportingToggle id on security page.')
      } else {
        safeBrowsingReportingToggle.setAttribute('hidden', 'true')
      }
    }
    const safeBrowsingEnhanced = templateContent.getElementById('safeBrowsingEnhanced')
    if (!safeBrowsingEnhanced) {
      console.error('[Brave Settings Overrides] Could not find safeBrowsingEnhanced id on security page.')
    } else {
      safeBrowsingEnhanced.setAttribute('hidden', 'true')
    }
    const safeBrowsingStandard =
      templateContent.getElementById('safeBrowsingStandard')
    if (!safeBrowsingStandard) {
      console.error(
        '[Brave Settings Overrides] Could not find safeBrowsingStandard id ' +
        'on security page.')
    } else {
      const passwordsLeakSettings = safeBrowsingStandard.
        querySelector(
          'template[is=dom-if][if="[[!enablePasswordLeakToggleMove_]]"]')
      if (!passwordsLeakSettings) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if=[[!enablePasswordLeakToggleMove_]] on security page.')
      } else {
        const passwordsLeakToggleOld = passwordsLeakSettings.content.
          getElementById('passwordsLeakToggleOld')
        if (!passwordsLeakToggleOld) {
          console.error(
            '[Brave Settings Overrides] Could not find passwordsLeakToggleOld ' +
            'on security page.')
        } else {
          passwordsLeakToggleOld.setAttribute('hidden', 'true')
        }
        // We don't want to show the separator or arrow icon for this
        // collapsible radio button, as we've hidden what's under it
        safeBrowsingStandard.setAttribute('no-collapse', 'true')
      }
    }
    if (loadTimeData.getBoolean("isHttpsByDefaultEnabled")) {
      const enableHttpsFirstModeNewSettings = templateContent.
        querySelector(
          'template[is=dom-if][if="[[!enableHttpsFirstModeNewSettings_]]"]'
      )
      if (!enableHttpsFirstModeNewSettings) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if=[[!enableHttpsFirstModeNewSettings]] on security page.')
      } else {
        const httpsOnlyModeToggle = enableHttpsFirstModeNewSettings.content.
          getElementById('httpsOnlyModeToggle')
        if (!httpsOnlyModeToggle) {
          console.error('[Brave Settings Overrides] Could not find' +
            'httpsOnlyModeToggle on security page.')
        } else {
          httpsOnlyModeToggle.setAttribute('hidden', 'true')
        }
      }
    }
    const link = templateContent.getElementById('advanced-protection-program-link')
    if (!link) {
      console.error('[Brave Settings Overrides] Could not find advanced-protection-program-link id on security page.')
    } else {
      link.setAttribute('hidden', 'true')
    }
    const enableCertManagementUIV2 = templateContent.
      querySelector(
        'template[is=dom-if][if="[[!enableCertManagementUIV2_]]"]'
    )
    if (!enableCertManagementUIV2) {
        console.error(
          '[Brave Settings Overrides] Could not find template with ' +
          'if=[[!enableCertManagementUIV2]] on security page.')
    } else {
      const chromeCertificates = enableCertManagementUIV2.content.
        getElementById('chromeCertificates')
      if (!chromeCertificates) {
        console.error(
          '[Brave Settings Overrides] Could not find chromeCertificates id ' +
          'on security page.')
      } else {
        chromeCertificates.setAttribute('hidden', 'true')
      }
    }
  }
})
