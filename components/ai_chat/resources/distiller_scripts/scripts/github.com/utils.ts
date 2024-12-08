/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export enum SupportedPage {
  ORG = 'org',
  REPO = 'repo',
  PULL_REQUESTS = 'pull_requests',
  PULL_REQUEST = 'pull_request',
  BRANCHES = 'branches',
  PROFILE = 'profile'
}

export function GetPageType(document: Document): SupportedPage | null {
  if (isOrgPage(document)) {
    return SupportedPage.ORG
  } else if (isRepoPage(document)) {
    return SupportedPage.REPO
  } else if (isPullRequestsPage(document)) {
    return SupportedPage.PULL_REQUESTS
  } else if (isPullRequestPage(document)) {
    return SupportedPage.PULL_REQUEST
  } else if (isBranchesPage(document)) {
    return SupportedPage.BRANCHES
  } else if (isProfilePage(document)) {
    return SupportedPage.PROFILE
  }

  return null
}

function isOrgPage(document: Document) {
  const key = 'itemtype'
  const val = 'Organization'
  return document.querySelector(`[${key}]`)?.getAttribute(key)?.endsWith(val)
}

function isRepoPage(document: Document) {
  return location.pathname.split('/').filter(Boolean).length === 2
}

function isPullRequestsPage(document: Document) {
  const pathParts = location.pathname.split('/').filter(Boolean)
  return pathParts.length === 3 && pathParts[2] === 'pulls'
}

function isPullRequestPage(document: Document) {
  const pathParts = location.pathname.split('/').filter(Boolean)
  return pathParts.length === 4 && /\d+/.test(pathParts[3])
}

function isBranchesPage(document: Document) {
  return /\/branches(\/(active|stale|all))?\/?$/.test(location.pathname)
}

function isProfilePage(document: Document) {
  return document.body.matches('.page-profile')
}
