// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html, nothing } from 'chrome://resources/lit/v3_0/lit.rollup.js'

import { RestartType } from '../relaunch_mixin_lit.js'
import { SettingsBraveOriginPageElement } from './brave_origin_page.js'

export function getHtml(this: SettingsBraveOriginPageElement) {
  return html`<!--_html_template_start_-->
    ${this.isPurchased_
      ? html`
          <div
            class="${this.getOriginRestartPaddingClass_(this.showRestartToast_)}"
            @policy-value-changed="${this.onPolicyValueChanged_}"
          >
            <settings-section
              id="origin"
              page-title="$i18n{braveOriginTitle}"
            >
              <div>
                <div class="settings-box first">
                  <div class="flex">
                    <div class="label primary-title">
                      $i18n{braveOriginOnboardingHeadingTitle}
                    </div>
                    <div class="label secondary secondary-title">
                      <p>$i18n{braveOriginOnboardingDescription}</p>
                    </div>
                  </div>
                </div>

                <if expr="enable_brave_rewards">
                  <div class="cr-row title">
                    $i18n{braveOriginSectionAdsTitle}
                  </div>

                  <origin-toggle-button
                    id="toggleRewardsButton"
                    policy-key="BraveRewardsDisabled"
                    inverted
                    label="$i18n{braveOriginRewardsToggleTitle}"
                    icon="product-bat-outline"
                  >
                  </origin-toggle-button>
                </if>

                <div class="cr-row title">
                  $i18n{braveOriginSectionAnalyticsTitle}
                </div>

                <origin-toggle-button
                  id="toggleP3AButton"
                  policy-key="BraveP3AEnabled"
                  label="$i18n{braveOriginP3AToggleTitle}"
                  sub-label="$i18n{braveOriginCrossProfileSubLabel}"
                  icon="bar-chart"
                >
                </origin-toggle-button>

                <origin-toggle-button
                  id="toggleStatsReportingButton"
                  policy-key="BraveStatsPingEnabled"
                  label="$i18n{braveOriginStatsReportingToggleTitle}"
                  sub-label="$i18n{braveOriginCrossProfileSubLabel}"
                  icon="bar-chart"
                >
                </origin-toggle-button>

                <if
                  expr="enable_ai_chat or enable_brave_news or enable_speedreader or enable_brave_talk or enable_tor or enable_brave_vpn or enable_brave_wallet or enable_brave_wayback_machine or enable_playlist or enable_email_aliases or enable_psst"
                >
                  <div class="cr-row title">
                    $i18n{braveOriginSectionFeaturesTitle}
                  </div>
                </if>

                <if expr="enable_ai_chat">
                  <origin-toggle-button
                    id="toggleLeoAiButton"
                    policy-key="BraveAIChatEnabled"
                    label="$i18n{braveOriginLeoAiToggleTitle}"
                    icon="product-brave-leo"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_local_ai">
                  <origin-toggle-button
                    id="toggleLocalAiButton"
                    policy-key="BraveLocalAIEnabled"
                    label="$i18n{braveOriginLocalAiToggleTitle}"
                    sub-label="$i18n{braveOriginCrossProfileSubLabel}"
                    icon="leo-local"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_brave_news">
                  <origin-toggle-button
                    id="toggleNewsButton"
                    policy-key="BraveNewsDisabled"
                    inverted
                    label="$i18n{braveOriginNewsToggleTitle}"
                    icon="product-brave-news"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_speedreader">
                  <origin-toggle-button
                    id="toggleSpeedreaderButton"
                    policy-key="BraveSpeedreaderEnabled"
                    label="$i18n{braveOriginSpeedReaderToggleTitle}"
                    icon="product-speedreader"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_playlist">
                  ${this.isPlaylistFeatureEnabled_
                    ? html`
                        <origin-toggle-button
                          id="togglePlaylistButton"
                          policy-key="BravePlaylistEnabled"
                          label="$i18n{braveOriginPlaylistToggleTitle}"
                          icon="product-playlist"
                        >
                        </origin-toggle-button>
                      `
                    : nothing}
                </if>

                <if expr="enable_brave_talk">
                  <origin-toggle-button
                    id="toggleTalkButton"
                    policy-key="BraveTalkDisabled"
                    inverted
                    label="$i18n{braveOriginTalkToggleTitle}"
                    icon="product-brave-talk"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_tor">
                  <origin-toggle-button
                    id="toggleTorWindowsButton"
                    policy-key="TorDisabled"
                    inverted
                    label="$i18n{braveOriginTorWindowsToggleTitle}"
                    sub-label="$i18n{braveOriginCrossProfileSubLabel}"
                    icon="product-tor"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_brave_vpn">
                  <origin-toggle-button
                    id="toggleVpnButton"
                    policy-key="BraveVPNDisabled"
                    inverted
                    label="$i18n{braveOriginVpnToggleTitle}"
                    icon="product-vpn"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_brave_wallet">
                  <origin-toggle-button
                    id="toggleWalletButton"
                    policy-key="BraveWalletDisabled"
                    inverted
                    label="$i18n{braveOriginWalletToggleTitle}"
                    icon="product-brave-wallet"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_brave_wayback_machine">
                  <origin-toggle-button
                    id="toggleWaybackMachineButton"
                    policy-key="BraveWaybackMachineEnabled"
                    label="$i18n{braveOriginWaybackMachineToggleTitle}"
                    icon="window-404"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_email_aliases">
                  <origin-toggle-button
                    id="toggleEmailAliasesButton"
                    policy-key="EmailAliasesEnabled"
                    label="$i18n{braveEmailAliasesToggleTitle}"
                    icon="email-shield"
                  >
                  </origin-toggle-button>
                </if>

                <if expr="enable_psst">
                  ${this.isPsstEnabled_
                    ? html`
                        <origin-toggle-button
                          id="togglePsstButton"
                          policy-key="PsstEnabled"
                          label="$i18n{bravePsstToggleTitle}"
                          sub-label="$i18n{bravePsstToggleSubLabel}"
                          icon="psst"
                        >
                        </origin-toggle-button>
                      `
                    : nothing}
                </if>

                <if expr="enable_web_discovery">
                  <origin-toggle-button
                    id="toggleWebDiscoveryProjectButton"
                    policy-key="BraveWebDiscoveryEnabled"
                    label="$i18n{braveOriginWebDiscoveryProjectToggleTitle}"
                    icon="brave-icon-search"
                  >
                  </origin-toggle-button>
                </if>

                <cr-link-row
                  id="resetToDefaults"
                  class="hr"
                  label="$i18n{braveOriginResetToDefaultsTitle}"
                  @click="${this.onResetToDefaultsClick_}"
                  start-icon="refresh"
                >
                </cr-link-row>
              </div>
            </settings-section>
          </div>
          ${this.showRestartToast_
            ? html`
                <div id="needsRestart">
                  <div class="flex-container">
                    <div class="flex restart-notice">
                      $i18n{braveOriginRestartNotice}
                    </div>
                    <div class="flex">
                      <leo-button
                        id="restartButton"
                        kind="filled"
                        @click="${this.onRestartClick_}"
                      >
                        $i18n{relaunchButtonLabel}
                      </leo-button>
                    </div>
                  </div>
                </div>
              `
            : nothing}
          ${this.shouldShowRelaunchDialog
            ? html`
                <relaunch-confirmation-dialog
                  .restartType="${RestartType.RESTART}"
                  @close="${this.onRelaunchDialogClose}"
                >
                </relaunch-confirmation-dialog>
              `
            : nothing}
        `
      : html`<settings-brave-origin-onboarding></settings-brave-origin-onboarding>`}
    <!--_html_template_end_-->`
}
