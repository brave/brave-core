/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { provideStrings } from '../../../../../.storybook/locale'

provideStrings({
  summarizeFailedLabel: 'The summarizer feature is currently available only for select articles and other long-form web pages.',
  placeholderLabel: 'Enter a prompt here',
  enableQuestionsTitle: 'Enable suggested questions?',
  enableQuestionsDesc: 'Brave AI can automatically suggest questions based on the content of the site. You can disable it later in Settings.',
  enableQuestionsButtonLabel: 'Enable',
  noThanksButtonLabel: 'No Thanks',
  aboutTitle: 'Brave AI',
  aboutDescription: 'The Brave AI Chat feature summarizes the content of a webpage you browse and includes a chat interface that allows you to make follow-up inquiries of the summarized content for that page.',
  aboutDescription_2: 'The summarized page and any questions you ask of the Chat feature are sent to the servers of an external API partner (no identifiers are sent with that query).',
  aboutNote: 'Please ensure that the page content and any follow-up inquiries do not contain personal or sensitive information. The accuracy of summaries and inquiry responses is not guaranteed and so you should not rely on any text related to health, personal safety or financial matters.',
  acceptButtonLabel: 'Accept and begin',
  pageContentWarning: 'Disconnect to stop sending this page content to Leo, and start a new conversation',
  errorNetworkLabel: 'There was a network issue connecting to Leo, check your connection and try again.',
  errorRateLimit: 'You\'ve reached the maximum number of questions for Leo. Please try again in a few hours.',
  retryButtonLabel: 'Retry'
})
