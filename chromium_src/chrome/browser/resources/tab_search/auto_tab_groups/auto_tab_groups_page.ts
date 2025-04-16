// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_button/cr_button.js'
import '//resources/cr_elements/cr_input/cr_input.js'

import {loadTimeData} from '//resources/js/load_time_data.js'
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js'

import {BraveTabSearchApiProxy, TabSearchApiProxyImpl} from '../tab_search_api_proxy.js'
import type {TabOrganizationSession} from '../tab_search.mojom-webui.js'

import {getHtml} from './auto_tab_groups_page.html.js'
import {getCss} from './auto_tab_groups_page.css.js'

export class AutoTabGroupsPageElement extends CrLitElement {
  static get is() {
    return 'auto-tab-groups-page'
  }

  protected topics_: string[] = []
  protected topic = ''
  protected undoTopic_ = ''
  protected showFRE_ =
      loadTimeData.getBoolean('showTabOrganizationFRE')

  private apiProxy_: BraveTabSearchApiProxy =
      TabSearchApiProxyImpl.getInstance() as BraveTabSearchApiProxy
  private listenerIds_: number[] = [];

  private visibilityChangedListener_: () => void
  private elementVisibilityChangedListener_: IntersectionObserver
  private isElementVisible_ = false

  static override get properties() {
    return {
      availableHeight: {type: Number},
      showBackButton: {type: Boolean},
      topic: {type: String},
      topics_: {type: Array},
      undoTopic_: {type: String},
      isLoadingTopics: {type: Boolean},
      errorMessage: {type: String},
      needsPremium: {type: Boolean},
      showFRE_: {type: Boolean},
    }
  }

  accessor availableHeight: number = 0
  accessor showBackButton: boolean = false
  accessor isLoadingTopics: boolean = false
  accessor errorMessage: string = ''
  accessor needsPremium: boolean = false

  static override get styles() {
    return getCss()
  }

  constructor() {
    super()

    this.visibilityChangedListener_ = this.maybeUpdateSuggestedTopics_

    this.elementVisibilityChangedListener_ =
        new IntersectionObserver((entries, _observer) => {
          this.onElementVisibilityChanged_((entries[0]?.intersectionRatio ?? 0) > 0);
        }, {root: document.documentElement})

  }

  protected onTopicInputChanged_(e: any) {
    this.topic = e.value
  }

  protected onFocusTabsClicked_() {
    this.getFocusTabs_(this.topic)
  }

  protected onUndoClicked_() {
    this.apiProxy_.undoFocusTabs().then(() => {
      this.undoTopic_ = ''
    })
  }

  private setShowFRE_(show: boolean) {
    this.showFRE_ = show
  }

  private getFocusTabs_(topic: string) {
    this.undoTopic_ = ''
    this.apiProxy_.getFocusTabs(topic).then(({windowCreated, error}) => {
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
    if (this.isLoadingTopics ||
        document.visibilityState !== 'visible' ||
        !this.isElementVisible_) {
      return
    }

    this.isLoadingTopics = true
    this.apiProxy_.getSuggestedTopics().then(({topics, error}) => {
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

  protected onTopicClicked_(index: number) {
    if (this.topics_[index]) {
      this.getFocusTabs_(this.topics_[index])
    }
  }

  override render() {
    return getHtml.bind(this)()
  }

  override connectedCallback() {
    super.connectedCallback()

    this.apiProxy_.getTabFocusShowFRE().then(
        ({showFRE}) => this.setShowFRE_(showFRE))

    const callbackRouter = this.apiProxy_.getCallbackRouter();
    this.listenerIds_.push(
        callbackRouter.showFREChanged.addListener(this.setShowFRE_.bind(this)))

    document.addEventListener('visibilitychange',
                              this.visibilityChangedListener_)
    this.elementVisibilityChangedListener_.observe(this)

    this.maybeUpdateSuggestedTopics_()
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    this.listenerIds_.forEach(
        id => this.apiProxy_.getCallbackRouter().removeListener(id))

    document.removeEventListener('visibilitychange',
                                 this.visibilityChangedListener_)
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

  protected getBackButtonAriaLabel_(): string {
    return loadTimeData.getStringF('backButtonAriaLabel', this.getTitle_())
  }

  protected getSubmitButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationSubmitButtonLabel')
  }

  protected getUndoButtonLabel_(): string {
    return loadTimeData.getString('tabOrganizationUndoButtonLabel')
  }

  protected getWindowCreatedMessage_(): string {
    return loadTimeData.getStringF('tabOrganizationWindowCreatedMessage', this.undoTopic_)
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

  protected onLearnMoreClicked_() {
    this.apiProxy_.openHelpPage()
  }

  protected onGoPremiumClicked_() {
    this.apiProxy_.openLeoGoPremiumPage()
  }

  protected onEnableTabFocusClicked_() {
    this.apiProxy_.setTabFocusEnabled()
  }

  protected onDismissErrorClicked_() {
    this.errorMessage = ''
  }

  override focus() {
    if (this.showBackButton) {
      const backButton = this.shadowRoot.querySelector('cr-icon-button')!
      if (backButton) {
        backButton.focus()
      }
    } else {
      super.focus()
    }
  }

  protected onBackClick_() {
    this.fire('back-click')
  }

  private onElementVisibilityChanged_(visible: boolean) {
    if (this.isElementVisible_ === visible) {
      return
    }

    this.isElementVisible_ = visible
    this.maybeUpdateSuggestedTopics_()
  }

  // Shim for Chromium auto_tab_groups_page test.
  setSessionForTesting(_session: TabOrganizationSession) {
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'auto-tab-groups-page': AutoTabGroupsPageElement
  }
}

customElements.define(AutoTabGroupsPageElement.is, AutoTabGroupsPageElement)
