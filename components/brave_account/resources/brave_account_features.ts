/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '//resources/js/load_time_data.js'
import {
  CrLitElement,
  PropertyValues,
} from '//resources/lit/v3_0/lit.rollup.js'

import { Service, SettingsPage } from './brave_account.mojom-webui.js'
import { getCss } from './brave_account_features.css.js'
import { getHtml } from './brave_account_features.html.js'
import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import {
  DependentServices,
  DependentServicesObserverCallbackRouter,
} from './dependent_services.mojom-webui.js'

export interface FeatureRow {
  icon: string
  label: string
  page: SettingsPage
}

function getFeatureRow(service: Service): FeatureRow | undefined {
  switch (service) {
    case Service.kEmailAliases:
      return {
        icon: 'email-shield',
        label: loadTimeData.getString(
          BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_FEATURES_EMAIL_ALIASES_LABEL,
        ),
        page: SettingsPage.kEmailAliases,
      }
    default:
      return undefined
  }
}

export class BraveAccountFeaturesElement extends CrLitElement {
  static get is() {
    return 'brave-account-features'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      rows: { type: Array, state: true },
    }
  }

  protected accessor rows: FeatureRow[] = []

  private observer = new DependentServicesObserverCallbackRouter()

  constructor() {
    super()

    this.observer.onDependentServicesChanged.addListener(
      (services: Service[]) => {
        this.rows = services
          .map(getFeatureRow)
          .filter((row): row is FeatureRow => !!row)
      },
    )
    DependentServices.getRemote().addObserver(
      this.observer.$.bindNewPipeAndPassRemote(),
    )
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)

    this.toggleAttribute('hidden', !this.rows.length)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-features': BraveAccountFeaturesElement
  }
}

customElements.define(
  BraveAccountFeaturesElement.is,
  BraveAccountFeaturesElement,
)
