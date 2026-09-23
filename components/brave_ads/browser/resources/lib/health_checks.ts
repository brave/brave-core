/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { AppState, Campaign, CampaignCreativeSet } from './app_store'
import { getDiagnosticValue, isWalletConnected } from './diagnostics'
import {
  formatRelativeDuration,
  nowInSeconds,
  splitTrailingParenthetical,
} from './format_time'
import * as routes from './app_routes'

export type HealthSeverity = 'info' | 'warning' | 'error'

export interface HealthCheck {
  id: string
  severity: HealthSeverity
  message: string
  route: string
}

// Past this many retries for a single confirmation, or this many items
// queued at once, something is likely stuck rather than just experiencing
// transient network hiccups.
const RETRY_COUNT_THRESHOLD = 5
const QUEUE_BACKLOG_THRESHOLD = 20

// A day is comfortably longer than the normal per-token redemption cadence,
// so still being overdue past this means redemption itself is stuck, not
// just waiting for the next scheduled attempt.
const PAYMENT_REDEMPTION_OVERDUE_SECONDS = 24 * 60 * 60

// Long enough that a transaction genuinely awaiting its next scheduled
// reconciliation attempt isn't mistaken for one that's stuck.
const TRANSACTION_UNRECONCILED_SECONDS = 7 * 24 * 60 * 60

// Matches `isErrorLine` in logs.tsx; scoped to the diagnostic log's most
// recent lines only (see `computeHealthChecks` below), and further filtered
// to lines that name a brave_ads source file, since the log is shared with
// Rewards' own (out of scope here) diagnostics.
function isAdsErrorLogLine(line: string) {
  return (line.includes('ERROR') || line.includes('Failed')) &&
    line.includes('brave_ads')
}

// Only the most recent lines are worth scanning; a failure from hours ago
// that hasn't recurred isn't an active problem.
const RECENT_LOG_LINE_COUNT = 200

function isMatch(entries: AppState['diagnosticEntries'], name: string) {
  return getDiagnosticValue(entries, name) === 'true'
}

// A 0 for any cap means unlimited (see the `Campaign`/`CampaignCreativeSet`
// interfaces), so such a cap never counts as hit no matter how many
// impressions have served.
function isCreativeSetAtCap(creativeSet: CampaignCreativeSet) {
  return (
    (creativeSet['Per Day'] > 0 &&
      creativeSet['Per Day Served'] >= creativeSet['Per Day']) ||
    (creativeSet['Per Week'] > 0 &&
      creativeSet['Per Week Served'] >= creativeSet['Per Week']) ||
    (creativeSet['Per Month'] > 0 &&
      creativeSet['Per Month Served'] >= creativeSet['Per Month']) ||
    (creativeSet['Total Max'] > 0 &&
      creativeSet['Total Max Served'] >= creativeSet['Total Max'])
  )
}

// A campaign can no longer serve any ad once its own daily cap is hit, or
// once every one of its creative sets has hit a cap of its own.
function isCampaignIneligible(campaign: Campaign) {
  if (campaign['Daily Cap'] > 0 &&
      campaign['Daily Cap Served'] >= campaign['Daily Cap']) {
    return true
  }
  return campaign['Creative Sets'].length > 0 &&
    campaign['Creative Sets'].every(isCreativeSetAtCap)
}

export function computeHealthChecks(state: AppState): HealthCheck[] {
  const checks: HealthCheck[] = []

  function add(
    id: string, severity: HealthSeverity, message: string, route: string,
  ) {
    checks.push({ id, severity, message, route })
  }

  const walletConnected = isWalletConnected(state.rewardsDiagnosticEntries)

  const notificationAdsEnabled =
    state.rewardsEnabled && isMatch(state.diagnosticEntries,
      'Notification ads enabled')
  const newTabPageAdsEnabled =
    isMatch(state.diagnosticEntries, 'New tab page ads shown')
  const searchResultAdsEnabled =
    isMatch(state.diagnosticEntries, 'Sponsored ads enabled') &&
      !walletConnected
  const anyAdFormatEnabled = notificationAdsEnabled || newTabPageAdsEnabled ||
    searchResultAdsEnabled

  // Service.
  if (state.diagnosticsLoaded && anyAdFormatEnabled && !state.isInitialized) {
    add('service-not-running', 'error',
      'The ads service should be running but isn\'t.', routes.diagnostics)
  }
  if (state.variationsCountryCode.endsWith('(Fallback)')) {
    add('variations-country-fallback', 'warning',
      'Ad targeting is using your device\'s locale for country because ' +
        'the Variations seed hasn\'t been fetched yet.', routes.diagnostics)
  }

  // Database.
  const migrationFailureReason = getDiagnosticValue(
    state.storageDiagnosticEntries, 'Last migration failure reason')
  if (migrationFailureReason && migrationFailureReason !== 'None') {
    add('migration-failed', 'error', 'Database migration failed.',
      routes.storage)
  }

  // Confirmation queue.
  const stuckRetries = state.confirmationQueue.filter(
    (item) => item['Retry Count'] >= RETRY_COUNT_THRESHOLD)
  if (stuckRetries.length > 0) {
    add('confirmation-retries', 'warning',
      `${stuckRetries.length} confirmation(s) have been retried ` +
        `${RETRY_COUNT_THRESHOLD}+ times.`, routes.confirmationQueue)
  }
  if (state.confirmationQueue.length >= QUEUE_BACKLOG_THRESHOLD) {
    add('confirmation-backlog', 'info',
      `${state.confirmationQueue.length} confirmations are queued.`,
      routes.confirmationQueue)
  }
  const now = nowInSeconds()
  const overdueItems = state.confirmationQueue.filter(
    (item) => item['Process At'] !== undefined && item['Process At'] < now)
  if (overdueItems.length > 0) {
    add('confirmation-stuck', 'warning',
      `${overdueItems.length} confirmation(s) are past their scheduled ` +
        'retry time.', routes.confirmationQueue)
  }

  // Tokens.
  const confirmationTokensRemaining = getDiagnosticValue(
    state.confirmationTokensDiagnosticEntries, 'Confirmation tokens remaining')
  if (state.isInitialized && confirmationTokensRemaining === '0') {
    add('confirmation-tokens-empty', 'warning',
      'No confirmation tokens remaining; new ad events can\'t be ' +
        'confirmed until more are refilled.', routes.confirmationTokens)
  }
  if (
    state.nextPaymentTokenRedemptionAt !== null &&
    now - state.nextPaymentTokenRedemptionAt >
      PAYMENT_REDEMPTION_OVERDUE_SECONDS
  ) {
    add('payment-redemption-overdue', 'warning',
      'Payment token redemption was due ' +
        `${formatRelativeDuration(state.nextPaymentTokenRedemptionAt)}.`,
      routes.paymentTokens)
  }

  // Resources.
  const purchaseIntentResource = getDiagnosticValue(
    state.resourcesDiagnosticEntries, 'Purchase intent resource')
  if (purchaseIntentResource === 'Failed to load') {
    add('purchase-intent-resource-failed', 'error',
      'The purchase intent resource failed to load.', routes.resources)
  }
  const antiTargetingResource = getDiagnosticValue(
    state.resourcesDiagnosticEntries, 'Anti targeting resource')
  if (antiTargetingResource === 'Failed to load') {
    add('anti-targeting-resource-failed', 'error',
      'The anti-targeting resource failed to load.', routes.resources)
  }
  const catalogLastUpdated = getDiagnosticValue(
    state.diagnosticEntries, 'Catalog last updated')
  if (catalogLastUpdated?.endsWith('overdue)')) {
    // e.g. "(3 days overdue)" -> "3 days". `parenthetical` can't actually be
    // null here since `endsWith('overdue)')` above guarantees a trailing
    // "(...)" group, but the return type doesn't know that.
    const { parenthetical } = splitTrailingParenthetical(catalogLastUpdated)
    const overdueBy = (parenthetical ?? '').replace(/^\(|\s*overdue\)$/g, '')
    add('catalog-overdue', 'warning',
      `The catalog hasn't updated in ${overdueBy}.`, routes.resources)
  }

  // Condition matchers.
  const invalidMatchers = state.conditionMatchers.filter(
    (matcher) => matcher.Matches === 'Invalid')
  if (invalidMatchers.length > 0) {
    add('condition-matchers-invalid', 'warning',
      `${invalidMatchers.length} condition matcher(s) are invalid.`,
      routes.conditionMatchers)
  }

  // Campaigns.
  if (notificationAdsEnabled &&
      state.activeNotificationAdCampaigns.length > 0 &&
      state.activeNotificationAdCampaigns.every(isCampaignIneligible)) {
    add('notification-ads-at-cap', 'info',
      'Every active notification ad campaign has hit its frequency caps; ' +
        'no notification ad is currently eligible to serve.', routes.campaigns)
  }
  if (newTabPageAdsEnabled &&
      state.activeNewTabPageAdCampaigns.length > 0 &&
      state.activeNewTabPageAdCampaigns.every(isCampaignIneligible)) {
    add('new-tab-page-ads-at-cap', 'info',
      'Every active new tab page ad campaign has hit its frequency caps; ' +
        'no new tab page ad is currently eligible to serve.', routes.campaigns)
  }
  if (newTabPageAdsEnabled && state.newTabPageAdGracePeriodEndAt !== null &&
      state.newTabPageAdGracePeriodEndAt > now) {
    add('new-tab-page-ads-grace-period', 'info',
      'New tab page ads are on hold during the grace period after a ' +
        'fresh install.',
      routes.campaigns)
  }
  const staleCampaigns = [
    ...state.activeNotificationAdCampaigns,
    ...state.activeNewTabPageAdCampaigns,
  ].filter((campaign) => campaign['End At'] > 0 && campaign['End At'] < now)
  if (staleCampaigns.length > 0) {
    add('stale-campaigns', 'info',
      `${staleCampaigns.length} active campaign(s) have already passed ` +
        'their end date.', routes.campaigns)
  }

  // Transactions.
  const stuckTransactions = state.transactions.filter(
    (transaction) =>
      transaction['Reconciled At'] === undefined &&
      transaction['Created At'] !== undefined &&
      now - transaction['Created At'] > TRANSACTION_UNRECONCILED_SECONDS)
  if (walletConnected && stuckTransactions.length === 1) {
    add('transactions-unreconciled', 'warning',
      '1 transaction hasn\'t reconciled since ' +
        `${formatRelativeDuration(stuckTransactions[0]['Created At']!)}.`,
      routes.transactions)
  } else if (walletConnected && stuckTransactions.length > 1) {
    add('transactions-unreconciled', 'warning',
      `${stuckTransactions.length} transactions haven't reconciled in ` +
        'over a week.',
      routes.transactions)
  }

  // Recent log activity.
  if (state.log) {
    const recentLines = state.log.split('\n').slice(-RECENT_LOG_LINE_COUNT)
    if (recentLines.some(isAdsErrorLogLine)) {
      add('recent-log-errors', 'error',
        'Recent log activity includes errors.', routes.logs)
    }
  }

  return checks
}
