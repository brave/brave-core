/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { formatDistanceToNow } from 'date-fns'

import { getLocale } from '../../common/locale'

interface Props {
  actions: any,
  subscriptions: AdBlock.SubscriptionInfo[]
}

interface State {
  currentlyAddingSubscription: boolean,
  newSubscriptionUrl: string,
  currentlyOpenedContextMenu: string | undefined
}

export class CustomSubscriptions extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentlyAddingSubscription: false,
      newSubscriptionUrl: "",
      currentlyOpenedContextMenu: undefined
    }
  }

  // A new subscription URL is invalid if it's not a URL, if it's using a
  // scheme other than HTTP or HTTPS,  or if it already exists within the
  // current list of subscriptions.
  newSubscriptionUrlValid = () => {
    try {
      const url = new URL(this.state.newSubscriptionUrl)
      if (url.protocol !== 'https:' && url.protocol !== 'http:') {
        return false
      }
      if (this.props.subscriptions.filter(subscription => subscription.subscription_url == url.href).length !== 0) {
        return false
      }
      return true
    } catch {}
    return false
  }

  onStartAddSubscription = (event: React.MouseEvent<HTMLButtonElement>) => {
    this.setState((state, props) => ({
      ...state,
      currentlyAddingSubscription: true
    }))
  }

  onCancelAddSubscription = (event: React.MouseEvent<HTMLButtonElement>) => {
    this.setState((state, props) => ({
      ...state,
      currentlyAddingSubscription: false,
      newSubscriptionUrl: ""
    }))
  }

  onChangeNewSubscriptionUrl = (event: React.ChangeEvent<HTMLInputElement>) => {
    const newSubscriptionUrl = event.target.value
    this.setState((state, props) => ({
      ...state,
      newSubscriptionUrl
    }))
  }

  onSubmitNewSubscription = (event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.submitNewSubscription(this.state.newSubscriptionUrl)
    this.setState((state, props) => ({
      ...state,
      currentlyAddingSubscription: false,
      newSubscriptionUrl: ""
    }))
  }

  onNewSubscriptionUrlKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      this.props.actions.submitNewSubscription(this.state.newSubscriptionUrl)
      this.setState((state, props) => ({
        ...state,
        currentlyAddingSubscription: false,
        newSubscriptionUrl: "",
      }))
    }
  }

  onToggleSubscription = (subscription_url: string, event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.setSubscriptionEnabled(subscription_url, event.target.checked)
  }

  onClickSubscriptionContextMenu = (subscription_url: string, event: React.MouseEvent<HTMLSpanElement>) => {
    const currentlyOpenedContextMenu = (this.state.currentlyOpenedContextMenu === subscription_url) ? undefined : subscription_url
    this.setState((state, props) => ({
      ...state,
      currentlyOpenedContextMenu
    }))
  }

  onRefreshSubscription = (subscription_url: string, event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.refreshSubscription(subscription_url)
    this.setState((state, props) => ({
      ...state,
      currentlyOpenedContextMenu: undefined
    }))
  }

  onUnsubscribe = (subscription_url: string, event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.deleteSubscription(subscription_url)
    this.setState((state, props) => ({
      ...state,
      currentlyOpenedContextMenu: undefined
    }))
  }

  onViewSource = (subscription_url: string, event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.viewSubscriptionSource(subscription_url)
  }

  renderTable = (subscriptions: AdBlock.SubscriptionInfo[]) => {
    const rows = subscriptions.map((subscription, i) => {
      const gridRow = i + 2

      // "Last updated" column can have four distinct states:
      // No update ever attempted: fine to show blank
      // Update attempted and failed, and never succeeded previously: show "Download failure", with last attempt on hover
      // Update attempted and failed, but succeeded previously: show "Download failure since " + the last successful time, with exact last successful time on hover
      // Update attempted and succeeded: show the last updated time, with exact time on hover
      let last_updated_cell
      if (subscription.last_update_attempt === 0) {
        last_updated_cell = (<div
            className="filterListGridCell"
            style={{gridRow, gridColumn: 2}}
          ></div>)
      } else if (subscription.last_successful_update_attempt === 0) {
        last_updated_cell = (<div
            className="filterListGridCell"
            style={{gridRow, gridColumn: 2, color: "#bd1531"}}
          >
            <span title={"Last attempted " + new Date(subscription.last_update_attempt).toISOString()}>{"Download failure"}</span>
          </div>)
      } else if (subscription.last_successful_update_attempt !== subscription.last_update_attempt) {
        last_updated_cell = (<div
            className="filterListGridCell"
            style={{gridRow, gridColumn: 2, color: "#bd1531"}}
          >
            <span title={"Last succeeded " + new Date(subscription.last_successful_update_attempt).toISOString() + ", attempted " + new Date(subscription.last_update_attempt).toISOString()}>{"Download failure since " + formatDistanceToNow(new Date(subscription.last_successful_update_attempt)) + " ago"}</span>
          </div>)
      } else {
        last_updated_cell = (<div
            className="filterListGridCell"
            style={{gridRow, gridColumn: 2}}
          >
            <span title={new Date(subscription.last_successful_update_attempt).toISOString()}>{formatDistanceToNow(new Date(subscription.last_successful_update_attempt)) + " ago"}</span>
          </div>)
      }

      return (<React.Fragment key={subscription.subscription_url}>
        <div style={{gridRow, gridColumn: "1 / span 4", borderBottom: "1px solid #e8e8e8"}}/>
        <div className="filterListGridCell" style={{gridRow, gridColumn: 1}}>{subscription.subscription_url}</div>
        { last_updated_cell }
        {
          subscription.last_successful_update_attempt ?
            (<div className="filterListGridCell" style={{gridRow, gridColumn: 3}}><input type="checkbox" checked={subscription.enabled} onChange={(e) => this.onToggleSubscription(subscription.subscription_url, e)}></input></div>) :
            (<></>)
        }
        <div className="filterListGridCell" style={{gridRow, gridColumn: "4 / span 5"}}>
          <span style={{cursor: "pointer", color: "#656565", userSelect: "none", fontSize: "1.25em"}} onClick={(e) => this.onClickSubscriptionContextMenu(subscription.subscription_url, e)}>•••</span>
          {
            (subscription.subscription_url === this.state.currentlyOpenedContextMenu) ?
              (<div style={{display: "flex", flexDirection: "column"}}>
                <button onClick={(e) => this.onRefreshSubscription(subscription.subscription_url, e)}>{getLocale('customListSubscriptionsTriggerUpdate')}</button>
                {
                  subscription.last_successful_update_attempt ?
                    <button onClick={(e) => this.onViewSource(subscription.subscription_url, e)}>{getLocale('customListSubscriptionsViewListSource')}</button> :
                    <></>
                }
                <button onClick={(e) => this.onUnsubscribe(subscription.subscription_url, e)}>{getLocale('customListSubscriptionsUnsubscribe')}</button>
              </div>) :
              (<></>)
          }
        </div>
      </React.Fragment>)
    })
    return (<>
      <style>.filterListGridCell {'{'} padding: 6px 15px {'}'}</style>
      <div style={{display: "grid", gridTemplateColumns: "50fr 30fr 3em 3em 9em"}}>
        <div style={{gridRow: 1, gridColumn: "1 / span 4", background: "#fafafa", borderBottom: "2px solid #e8e8e8", borderRadius: "4px 4px 0 0"}}/>
        <div className="filterListGridCell" style={{gridRow: 1, gridColumn: 1, fontWeight: "bold"}}>{getLocale('customListSubscriptionsTableFilterListColumn')}</div>
        <div className="filterListGridCell" style={{gridRow: 1, gridColumn: 2, fontWeight: "bold"}}>{getLocale('customListSubscriptionsTableLastUpdatedColumn')}</div>
        { rows }
      </div>
    </>)
  }

  render () {
    const addSubscriptionSection = this.state.currentlyAddingSubscription ?
      (<>
        <input autoFocus value={this.state.newSubscriptionUrl} onChange={this.onChangeNewSubscriptionUrl} onKeyDown={this.onNewSubscriptionUrlKeyDown} placeholder={getLocale('customListSubscriptionsEnterSubscriptionUrlPlaceholder')}></input>
        <button disabled={!this.newSubscriptionUrlValid()} onClick={this.onSubmitNewSubscription}>{getLocale('customListSubscriptionsSubmitNewSubscription')}</button>
        <button onClick={this.onCancelAddSubscription}>{getLocale('customListSubscriptionsCancelAddSubscription')}</button>
      </>) :
      (<button onClick={this.onStartAddSubscription}>{getLocale('customListSubscriptionsAddNewFilterList')}</button>)

    const existingListsSection = this.props.subscriptions.length === 0 ? (<></>) : this.renderTable(this.props.subscriptions)

    return (
      <div>
        <div
          i18n-content='customListSubscriptionsTitle'
          style={{ fontSize: '18px', marginTop: '20px' }}
        />
        <div
          i18n-content='customListSubscriptionsInstructions'
        />
        {existingListsSection}
        {addSubscriptionSection}
        <div
          i18n-content='customListSubscriptionsDisclaimer'
        />
      </div>
    )
  }
}
