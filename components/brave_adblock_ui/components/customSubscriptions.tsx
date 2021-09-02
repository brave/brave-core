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
      newSubscriptionUrl: '',
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
      if (this.props.subscriptions.filter(subscription => subscription.subscription_url === url.href).length !== 0) {
        return false
      }
      return true
    } catch {
      /* fall through if URL constructor fails */
    }
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
      newSubscriptionUrl: ''
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
      newSubscriptionUrl: ''
    }))
  }

  onNewSubscriptionUrlKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && this.newSubscriptionUrlValid()) {
      this.props.actions.submitNewSubscription(this.state.newSubscriptionUrl)
      this.setState((state, props) => ({
        ...state,
        currentlyAddingSubscription: false,
        newSubscriptionUrl: ''
      }))
    }
  }

  onToggleSubscription = (subscriptionUrl: string, event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.setSubscriptionEnabled(subscriptionUrl, event.target.checked)
  }

  onClickSubscriptionContextMenu = (subscriptionUrl: string, event: React.MouseEvent<HTMLSpanElement>) => {
    const currentlyOpenedContextMenu = (this.state.currentlyOpenedContextMenu === subscriptionUrl) ? undefined : subscriptionUrl
    this.setState((state, props) => ({
      ...state,
      currentlyOpenedContextMenu
    }))
  }

  onRefreshSubscription = (subscriptionUrl: string, event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.refreshSubscription(subscriptionUrl)
    this.setState((state, props) => ({
      ...state,
      currentlyOpenedContextMenu: undefined
    }))
  }

  onUnsubscribe = (subscriptionUrl: string, event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.deleteSubscription(subscriptionUrl)
    this.setState((state, props) => ({
      ...state,
      currentlyOpenedContextMenu: undefined
    }))
  }

  onViewSource = (subscriptionUrl: string, event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.actions.viewSubscriptionSource(subscriptionUrl)
  }

  renderTable = (subscriptions: AdBlock.SubscriptionInfo[]) => {
    const rows = subscriptions.map((subscription, i) => {
      const gridRow = i + 2

      // "Last updated" column can have four distinct states:
      // No update ever attempted: fine to show blank
      // Update attempted and failed, and never succeeded previously: show "Download failure", with last attempt on hover
      // Update attempted and failed, but succeeded previously: show "Download failure since " + the last successful time, with exact last successful time on hover
      // Update attempted and succeeded: show the last updated time, with exact time on hover
      let lastUpdatedCell
      if (subscription.last_update_attempt === 0) {
        lastUpdatedCell = (
          <div
            className='filterListGridCell'
            style={{ gridRow, gridColumn: 2 }}
          />
        )
      } else if (subscription.last_successful_update_attempt === 0) {
        lastUpdatedCell = (
          <div
            className='filterListGridCell'
            style={{ gridRow, gridColumn: 2, color: '#bd1531' }}
          >
            <span title={'Last attempted ' + new Date(subscription.last_update_attempt).toISOString()}>{'Download failure'}</span>
          </div>
        )
      } else if (subscription.last_successful_update_attempt !== subscription.last_update_attempt) {
        lastUpdatedCell = (
          <div
            className='filterListGridCell'
            style={{ gridRow, gridColumn: 2, color: '#bd1531' }}
          >
            <span title={'Last succeeded ' + new Date(subscription.last_successful_update_attempt).toISOString() + ', attempted ' + new Date(subscription.last_update_attempt).toISOString()}>{'Download failure since ' + formatDistanceToNow(new Date(subscription.last_successful_update_attempt)) + ' ago'}</span>
          </div>
        )
      } else {
        lastUpdatedCell = (
          <div
            className='filterListGridCell'
            style={{ gridRow, gridColumn: 2 }}
          >
            <span title={new Date(subscription.last_successful_update_attempt).toISOString()}>{formatDistanceToNow(new Date(subscription.last_successful_update_attempt)) + ' ago'}</span>
          </div>
        )
      }

      const onToggleThisSubscription = (e: React.ChangeEvent<HTMLInputElement>) => this.onToggleSubscription(subscription.subscription_url, e)
      const onClickThisContextMenu = (e: React.MouseEvent<HTMLSpanElement, MouseEvent>) => this.onClickSubscriptionContextMenu(subscription.subscription_url, e)
      const onRefreshThisSubscription = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => this.onRefreshSubscription(subscription.subscription_url, e)
      const onViewThisSource = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => this.onViewSource(subscription.subscription_url, e)
      const onUnsubscribeThisList = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => this.onUnsubscribe(subscription.subscription_url, e)

      return (
        <React.Fragment key={subscription.subscription_url}>
          <div style={{ gridRow, gridColumn: '1 / span 4', borderBottom: '1px solid #e8e8e8' }}/>
          <div className='filterListGridCell' style={{ gridRow, gridColumn: 1 }}>{subscription.subscription_url}</div>
          {lastUpdatedCell}
          {
            subscription.last_successful_update_attempt ?
              (<div className='filterListGridCell' style={{ gridRow, gridColumn: 3 }}><input type='checkbox' checked={subscription.enabled} onChange={onToggleThisSubscription}/></div>) :
              (<></>)
          }
          <div className='filterListGridCell' style={{ gridRow, gridColumn: '4 / span 5' }}>
            <span style={{ cursor: 'pointer', color: '#656565', userSelect: 'none', fontSize: '1.25em' }} onClick={onClickThisContextMenu}>•••</span>
            {
              (subscription.subscription_url === this.state.currentlyOpenedContextMenu) ?
                (<div style={{ display: 'flex', flexDirection: 'column' }}>
                  <button onClick={onRefreshThisSubscription}>{getLocale('customListSubscriptionsTriggerUpdate')}</button>
                  {
                    subscription.last_successful_update_attempt ?
                      <button onClick={onViewThisSource}>{getLocale('customListSubscriptionsViewListSource')}</button> :
                      <></>
                  }
                  <button onClick={onUnsubscribeThisList}>{getLocale('customListSubscriptionsUnsubscribe')}</button>
                </div>) :
                (<></>)
            }
          </div>
        </React.Fragment>
      )
    })
    return (
      <>
        <style>.filterListGridCell {'{'} padding: 6px 15px {'}'}</style>
        <div style={{ display: 'grid', gridTemplateColumns: '50fr 30fr 3em 3em 9em' }}>
          <div style={{ gridRow: 1, gridColumn: '1 / span 4', background: '#fafafa', borderBottom: '2px solid #e8e8e8', borderRadius: '4px 4px 0 0' }}/>
          <div className='filterListGridCell' style={{ gridRow: 1, gridColumn: 1, fontWeight: 'bold' }}>{getLocale('customListSubscriptionsTableFilterListColumn')}</div>
          <div className='filterListGridCell' style={{ gridRow: 1, gridColumn: 2, fontWeight: 'bold' }}>{getLocale('customListSubscriptionsTableLastUpdatedColumn')}</div>
          {rows}
        </div>
      </>
    )
  }

  render () {
    const addSubscriptionSection = this.state.currentlyAddingSubscription ?
      (
        <>
          <input autoFocus={true} value={this.state.newSubscriptionUrl} onChange={this.onChangeNewSubscriptionUrl} onKeyDown={this.onNewSubscriptionUrlKeyDown} placeholder={getLocale('customListSubscriptionsEnterSubscriptionUrlPlaceholder')}/>
          <button disabled={!this.newSubscriptionUrlValid()} onClick={this.onSubmitNewSubscription}>{getLocale('customListSubscriptionsSubmitNewSubscription')}</button>
          <button onClick={this.onCancelAddSubscription}>{getLocale('customListSubscriptionsCancelAddSubscription')}</button>
        </>
      ) :
      (<button onClick={this.onStartAddSubscription}>{getLocale('customListSubscriptionsAddNewFilterList')}</button>)

    const existingListsSection = this.props.subscriptions.length === 0 ? (<></>) : this.renderTable(this.props.subscriptions)

    return (
      <div>
        <div
          style={{ fontSize: '18px', marginTop: '20px' }}
        >
          {getLocale('customListSubscriptionsTitle')}
        </div>
        <div>
          {getLocale('customListSubscriptionsInstructions')}
        </div>
        {existingListsSection}
        {addSubscriptionSection}
        <div>
          {getLocale('customListSubscriptionsDisclaimer')}
        </div>
      </div>
    )
  }
}
