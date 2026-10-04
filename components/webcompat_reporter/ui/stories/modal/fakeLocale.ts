/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const locale: { [key: string]: string } = {
  // Report modal
  reportModalTitle: 'Report a broken site',
  reportExplanation: 'Let Brave\'s developers know that this site doesn\'t work properly with Shields:',
  reportDisclaimer: 'Note: The report sent to Brave servers will include the site address, Brave version number, Shields settings, VPN status and language settings. The IP address will be transmitted but not stored.',
  reportDetails: 'Additional details (optional)',
  reportContactPlaceholder: 'Email, Twitter, etc.',
  reportContactLabel: 'Contact me at: (optional)',
  cancel: 'Cancel',
  submit: 'Submit',
  // Confirmation modal
  thankYou: 'Thank you!',
  confirmationNotice: 'Thanks for letting Brave\'s developers know that there\'s something wrong with this site. We\'ll do our best to fix it!'
}

export default locale

export const getLocale = (word: string) => locale[word]
