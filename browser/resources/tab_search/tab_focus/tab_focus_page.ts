// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_button/cr_button.js'
import '//resources/cr_elements/cr_input/cr_input.js'

import { loadTimeData } from '//resources/js/load_time_data.js'
import { CrLitElement, html } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveTabSearchApiProxy,
  TabSearchApiProxyImpl,
} from '../tab_search_api_proxy.js'

import { getHtml } from './tab_focus_page.html.js'
import { getCss } from './tab_focus_page.css.js'

export class TabFocusPageElement extends CrLitElement {
  static get is() {
    return 'tab-focus-page'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  private apiProxy_: BraveTabSearchApiProxy =
    TabSearchApiProxyImpl.getInstance() as BraveTabSearchApiProxy
  private listenerIds_: number[] = []

  private visibilityChangedListener_: () => void
  private elementVisibilityChangedListener_: IntersectionObserver
  private isElementVisible_ = false

  static override get properties() {
    return {
      availableHeight: { type: Number },
      topic: { type: String },
      topics_: { type: Array },
      undoTopic_: { type: String },
      isLoadingTopics: { type: Boolean },
      errorMessage: { type: String },
      needsPremium: { type: Boolean },
      showFRE_: { type: Boolean },
    }
  }

  protected accessor topics_: string[] = []
  protected accessor topic: string = ''
  protected accessor undoTopic_: string = ''
  protected accessor showFRE_: boolean = loadTimeData.getBoolean(
    'showTabOrganizationFRE',
  )

  accessor availableHeight: number = 0
  accessor isLoadingTopics: boolean = false
  accessor errorMessage: string = ''
  accessor needsPremium: boolean = false

  constructor() {
    super()

    this.visibilityChangedListener_ = this.maybeUpdateSuggestedTopics_

    this.elementVisibilityChangedListener_ = new IntersectionObserver(
      (entries, _observer) => {
        this.onElementVisibilityChanged_(
          (entries[0]?.intersectionRatio ?? 0) > 0,
        )
      },
      { root: document.documentElement },
    )
  }

  protected onTopicInputChanged_(e: any) {
    this.topic = e.value
  }

  protected onFocusTabsClick_() {
    this.getFocusTabs_(this.topic)
  }

  protected onUndoClick_() {
    this.apiProxy_.undoFocusTabs().then(() => {
      this.undoTopic_ = ''
    })
  }

  private setShowFRE_(show: boolean) {
    this.showFRE_ = show
  }

  private getFocusTabs_(topic: string) {
    this.undoTopic_ = ''
    this.apiProxy_.getFocusTabs(topic).then(({ windowCreated, error }) => {
      if (error) {
        this.errorMessage = error.message
        if (error.rateLimitedInfo) {
          this.needsPremium = !error.rateLimitedInfo.isPremium
        }
        return
      }
      this.errorMessage = ''
      if (windowCreated) {
        this.undoTopic_ = topic
      }
    })
  }

  private maybeUpdateSuggestedTopics_ = () => {
    // Don't update if it's already loading topics or when it's not visible.
    if (
      this.isLoadingTopics
      || document.visibilityState !== 'visible'
      || !this.isElementVisible_
    ) {
      return
    }

    this.isLoadingTopics = true
    this.apiProxy_.getSuggestedTopics().then(({ topics, error }) => {
      this.isLoadingTopics = false
      if (error) {
        this.errorMessage = error.message
        if (error.rateLimitedInfo) {
          this.needsPremium = !error.rateLimitedInfo.isPremium
        }
        return
      }
      this.onSuggestTopicsUpdated(topics)
      this.errorMessage = ''
    })
  }

  private onSuggestTopicsUpdated(topics: string[]) {
    this.topics_ = topics
  }

  protected getTopicId_(index: number): string {
    return 'topicId' + index
  }

  protected getTopicWithoutEmoji_(topic: string): string {
    return topic.replace(/[^\x20-\x7F]/g, '').trimStart()
  }

  protected getTopicEmoji_(topic: string): string {
    return topic.match(/^\p{Emoji}/u)![0]
  }

  protected onTopicClick_(index: number) {
    if (this.topics_[index]) {
      this.getFocusTabs_(this.topics_[index])
    }
  }

  override connectedCallback() {
    super.connectedCallback()

    this.apiProxy_
      .getTabFocusShowFRE()
      .then(({ showFRE }) => this.setShowFRE_(showFRE))

    const callbackRouter = this.apiProxy_.getCallbackRouter()
    this.listenerIds_.push(
      callbackRouter.tabFocusShowFREChanged.addListener(
        this.setShowFRE_.bind(this),
      ),
    )

    document.addEventListener(
      'visibilitychange',
      this.visibilityChangedListener_,
    )
    this.elementVisibilityChangedListener_.observe(this)

    this.maybeUpdateSuggestedTopics_()
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    this.listenerIds_.forEach((id) =>
      this.apiProxy_.getCallbackRouter().removeListener(id),
    )

    document.removeEventListener(
      'visibilitychange',
      this.visibilityChangedListener_,
    )
    this.elementVisibilityChangedListener_.disconnect()
  }

  protected getTitle_(): string {
    return loadTimeData.getString('tabOrganizationTitle')
  }

  protected getSubtitle_(): string {
    return loadTimeData.getString('tabOrganizationSubtitle')
  }

  protected getSuggestedTopicsSubtitle_(): string {
    return loadTimeData.getString('tabOrganizationSuggestedTopicsSubtitle')
  }

  protected getSubmitButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationSubmitButtonLabel')
  }

  protected getUndoButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationUndoButtonLabel')
  }

  protected getWindowCreatedMessage_(): string {
    return loadTimeData.getStringF(
      'tabOrganizationWindowCreatedMessage',
      this.undoTopic_,
    )
  }

  protected getTopicInputPlaceholder_(): string {
    return loadTimeData.getString('tabOrganizationTopicInputPlaceholder')
  }

  protected getFooterMessage_(): string {
    return loadTimeData.getStringF('tabOrganizationSendTabDataMessage')
  }

  protected getLearnMoreLabel_(): string {
    return loadTimeData.getString('tabOrganizationLearnMoreLabel')
  }

  protected getGoPremiumButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationGoPremiumButtonLabel')
  }

  protected getDismissButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationDismissButtonLabel')
  }

  protected getPrivacyDisclaimerMessage_(): string {
    return loadTimeData.getString('tabOrganizationPrivacyDisclaimer')
  }

  protected getEnableButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationEnableButtonLabel')
  }

  protected onLearnMoreClick_() {
    this.apiProxy_.openLearnMorePage()
  }

  protected onGoPremiumClick_() {
    this.apiProxy_.openLeoGoPremiumPage()
  }

  protected onEnableTabFocusClick_() {
    this.apiProxy_.setTabFocusEnabled()
  }

  protected onDismissErrorClick_() {
    this.errorMessage = ''
  }

  protected getUndoFocusTabsHtml_() {
    return this.undoTopic_ !== ''
      ? html`
          <leo-alert
            id="undo"
            type="success"
            hideIcon="{true}"
          >
            ${this.getWindowCreatedMessage_()}
            <leo-button
              id="undoButton"
              kind="plain-faint"
              size="tiny"
              @click="${this.onUndoClick_}"
            >
              ${this.getUndoButtonLabel_()}
            </leo-button>
          </leo-alert>
        `
      : ''
  }

  protected getTopicsHtml_() {
    return this.isLoadingTopics
      ? [1, 2, 3, 4, 5].map(
          (key) => html`
            <leo-button
              key=${key}
              class="topics-button"
              size="small"
              kind="outline"
              isDisabled="{true}"
            >
              <div class="topic-description">
                <div class="emoji-wrapper">
                  <leo-progressring class="loading-ring"></leo-progressring>
                </div>
                <div class="empty-state"></div>
              </div>
            </leo-button>
          `,
        )
      : this.topics_.map(
          (entry, index) => html`
            <leo-button
              id="${this.getTopicId_(index)}"
              data-testid="${this.getTopicId_(index)}"
              class="topics-button"
              size="small"
              kind="outline"
              @click="${() => this.onTopicClick_(index)}"
            >
              <div class="topic-description">
                <div class="emoji-wrapper">${this.getTopicEmoji_(entry)}</div>
                ${this.getTopicWithoutEmoji_(entry)}
              </div>
            </leo-button>
          `,
        )
  }

  protected getHeaderHtml_() {
    return html`
      <div class="title-row">
        <div class="title-column">
          <div class="title">${this.getTitle_()}</div>
          ${!this.showFRE_
            ? html` <div class="subtitle">${this.getSubtitle_()}</div> `
            : ''}
        </div>
      </div>
    `
  }

  protected getEnableTabFocusHtml_() {
    return html`
      <div class="enable-tab-focus-wrapper">
        ${this.getHeaderHtml_()}
        <div class="enable-tab-focus-content-wrapper">
          <div class="enable-tab-focus-illustration"></div>
          <div class="enable-tab-focus-info-wrapper">
            <span class="enable-tab-focus-info-text">
              ${this.getPrivacyDisclaimerMessage_()}
            </span>
            <div class="enable-tab-focus-button-row">
              <span
                class="learn-more-link"
                @click=${this.onLearnMoreClick_}
              >
                ${this.getLearnMoreLabel_()}
              </span>
              <div>
                <leo-button
                  id="enableButton"
                  data-testid="enable-tab-focus"
                  kind="filled"
                  size="small"
                  @click="${this.onEnableTabFocusClick_}"
                >
                  ${this.getEnableButtonLabel_()}
                </leo-button>
              </div>
            </div>
          </div>
        </div>
      </div>
    `
  }

  private onElementVisibilityChanged_(visible: boolean) {
    if (this.isElementVisible_ === visible) {
      return
    }

    this.isElementVisible_ = visible
    this.maybeUpdateSuggestedTopics_()
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'tab-focus-page': TabFocusPageElement
  }
}

customElements.define(TabFocusPageElement.is, TabFocusPageElement)
