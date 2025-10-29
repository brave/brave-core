// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { provideStrings } from '../../../../../.storybook/locale'

export function SetupEmailAliasesStrings() {
  provideStrings({
    emailAliasesShortDescription: 'Keep your personal email address private',
    emailAliasesDescription:
      'Create unique, random addresses that forward to your Brave account '
      + 'email and can be deleted at any time. Keep your actual email address '
      + 'from being disclosed or used by advertisers.',
    emailAliasesLearnMore: 'Learn More',
    emailAliasesSignOut: 'Sign Out',
    emailAliasesSignOutTitle: 'Sign Out of Email Aliases',
    emailAliasesConnectingToBraveAccount: 'Connecting to Brave Account...',
    emailAliasesBraveAccount: 'Brave Account',
    emailAliasesCopiedToClipboard: 'Copied to clipboard',
    emailAliasesClickToCopyAlias: 'Click to copy alias',
    emailAliasesUsedBy: 'Used by $1',
    emailAliasesEdit: 'Edit',
    emailAliasesDelete: 'Delete',
    emailAliasesCreateDescription:
      'Create up to 5 free email aliases to '
      + 'protect your real email address.',
    emailAliasesListTitle: 'Your Email aliases',
    emailAliasesCreateAliasTitle: 'New email alias',
    emailAliasesCreateAliasLabel: 'New alias',
    emailAliasesRefreshButtonTitle: 'Suggest another email alias',
    emailAliasesGeneratingNewAlias: 'Generating new alias...',
    emailAliasesGenerateError:
      'Error generating alias. Check your internet '
      + 'connection and try again.',
    emailAliasesNoteLabel: 'Note',
    emailAliasesEditNotePlaceholder: 'Enter a note for your address (optional)',
    emailAliasesCancelButton: 'Cancel',
    emailAliasesManageButton: 'Manage',
    emailAliasesAliasLabel: 'Email alias',
    emailAliasesEmailsWillBeForwardedTo: 'Emails will be forwarded to $1',
    emailAliasesEditAliasTitle: 'Edit email alias',
    emailAliasesCreateAliasButton: 'Create alias',
    emailAliasesUpdateAliasError:
      'Error saving alias. Check your internet ' + 'connection and try again.',
    emailAliasesSaveAliasButton: 'Save',
    emailAliasesDeleteAliasTitle: 'Delete email alias',
    emailAliasesDeleteAliasDescription:
      'Removing $1 as an alias will result '
      + 'in the loss of your ability to receive emails.',
    emailAliasesDeleteWarning:
      'This action is irreversible. Please ensure you '
      + 'want to proceed before continuing.',
    emailAliasesDeleteAliasButton: 'Delete',
    emailAliasesDeleteAliasError:
      'Error deleting alias. Check your internet '
      + 'connection and try again.',
    emailAliasesSignInOrCreateAccount:
      'To get started, sign in or create a ' + 'Brave account',
    emailAliasesEnterEmailToGetLoginLink:
      'Enter your email address to get a '
      + 'secure login link sent to your email. Clicking this link will either '
      + 'create or access a Brave Account and let you use the free Email Aliases '
      + 'service.',
    emailAliasesGetLoginLinkButton: 'Get login link',
    emailAliasesRequestAuthenticationError:
      'Error requesting authentication. Check your internet connection and try '
      + 'again.',
    emailAliasesEmailAddressPlaceholder: 'Email address',
    emailAliasesLoginEmailOnTheWay: 'A login email is on the way to $1',
    emailAliasesClickOnSecureLogin:
      'Click on the secure login link in the '
      + 'email to access your account.',
    emailAliasesDontSeeEmail:
      "Don't see the email? Check your spam folder " + 'or try again.',
    emailAliasesAuthError:
      'Error authenticating with Brave Account. ' + 'Please try again.',
    emailAliasesAuthTryAgainButton: 'Start over',
    emailAliasesBubbleDescription:
      'Create a random email address that '
      + 'forwards to your inbox while keeping your personal email private.',
    emailAliasesBubbleLimitReached:
      'You have reached the limit of 5 free '
      + 'email aliases. Click "Manage" to re-use or delete an alias.',
  })
}
