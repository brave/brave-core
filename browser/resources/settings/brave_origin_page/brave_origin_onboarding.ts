// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import '../settings_shared.css.js';

import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {BaseMixin} from '../base_mixin.js';
import {getTemplate} from './brave_origin_onboarding.html.js';

const SettingsBraveOriginOnboardingElementBase =
    BaseMixin(I18nMixin(PolymerElement)) as {
      new (): PolymerElement & I18nMixinInterface
    };

/**
 * 'settings-brave-origin-onboarding' is the settings section for
 * introducing Brave Origin to users who don't have it enabled yet.
 */
export class SettingsBraveOriginOnboardingElement extends
    SettingsBraveOriginOnboardingElementBase {
  static get is() {
    return 'settings-brave-origin-onboarding';
  }

  static get template() {
    return getTemplate();
  }

  private onBuyNowClick_() {
    window.open(
        'https://account.brave.com/?intent=checkout&product=origin&mtm_campaign=browser-settings',
        '_blank',
        'noopener');
  }

  private onLearnMoreClick_() {
    window.open(
        'https://support.brave.app/hc/en-us/articles/38561489788173',
        '_blank',
        'noopener');
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-origin-onboarding': SettingsBraveOriginOnboardingElement;
  }
}

customElements.define(
    SettingsBraveOriginOnboardingElement.is,
    SettingsBraveOriginOnboardingElement);
