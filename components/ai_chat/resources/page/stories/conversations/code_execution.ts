// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import { createConversationTurnWithDefaults } from '../../../common/test_data_utils'

const history: Mojom.ConversationTurn[] = [
  createConversationTurnWithDefaults({
    'uuid': '6a20ec93-a8ab-432a-b886-d14ef28c0750',
    'characterType': 0,
    'actionType': 5,
    'text':
      'I am thinking of opening a 25 year home mortgage worth 500,000 dollars. I have a 150,000 dollar downpayment. The interest rate is 4.9%. What would my typical monthly payment be and how much interest would I pay over the life of the mortgage?\n',
    'prompt': undefined,
    'selectedText': undefined,
    'events': undefined,
    'createdTime': { 'internalValue': BigInt('13421965400704068') },
    'edits': undefined,
    'uploadedFiles': undefined,
    'skill': undefined,
    'fromBraveSearchSERP': false,
    'modelKey': undefined,
    'nearVerificationStatus': undefined,
  }),

  createConversationTurnWithDefaults({
    'uuid': '2560d3df-3660-4d48-8f81-63ae2ec32515',
    'characterType': 1,
    'actionType': 1,
    'text': '',
    'prompt': undefined,
    'selectedText': undefined,
    'events': [
      {
        'completionEvent': undefined,
        'searchQueriesEvent': undefined,
        'searchStatusEvent': undefined,
        'conversationTitleEvent': undefined,
        'sourcesEvent': undefined,
        'contentReceiptEvent': undefined,
        'toolUseEvent': {
          'toolName': 'code_execution_tool',
          'id': 'tooluse_yDU9JfLQ9agIJL3DDSEG55',
          'argumentsJson':
            "{\"script\": \"\\n// Mortgage calculation\\nconst homePrice = 500000;\\nconst downPayment = 150000;\\nconst loanAmount = homePrice - downPayment;\\nconst annualRate = 4.9;\\nconst monthlyRate = annualRate / 100 / 12;\\nconst numberOfPayments = 25 * 12; // 25 years\\n\\n// Monthly payment formula: M = P * [r(1+r)^n] / [(1+r)^n - 1]\\nconst monthlyPayment = loanAmount * (monthlyRate * Math.pow(1 + monthlyRate, numberOfPayments)) / (Math.pow(1 + monthlyRate, numberOfPayments) - 1);\\n\\n// Total paid over life of mortgage\\nconst totalPaid = monthlyPayment * numberOfPayments;\\n\\n// Total interest paid\\nconst totalInterest = totalPaid - loanAmount;\\n\\nconsole.log(`Loan Amount: $${loanAmount.toLocaleString('en-US', {minimumFractionDigits: 2, maximumFractionDigits: 2})}`);\\nconsole.log(`Monthly Payment: $${monthlyPayment.toLocaleString('en-US', {minimumFractionDigits: 2, maximumFractionDigits: 2})}`);\\nconsole.log(`Total Paid Over 25 Years: $${totalPaid.toLocaleString('en-US', {minimumFractionDigits: 2, maximumFractionDigits: 2})}`);\\nconsole.log(`Total Interest Paid: $${totalInterest.toLocaleString('en-US', {minimumFractionDigits: 2, maximumFractionDigits: 2})}`);\\n\"}",
          'output': [
            {
              'imageContentBlock': undefined,
              'textContentBlock': {
                'text':
                  'Loan Amount: $350,000.00\nMonthly Payment: $2,025.73\nTotal Paid Over 25 Years: $607,717.54\nTotal Interest Paid: $257,717.54',
              },
              'fileContentBlock': undefined,
              'pageExcerptContentBlock': undefined,
              'pageTextContentBlock': undefined,
              'videoTranscriptContentBlock': undefined,
              'requestTitleContentBlock': undefined,
              'changeToneContentBlock': undefined,
              'memoryContentBlock': undefined,
              'filterTabsContentBlock': undefined,
              'suggestFocusTopicsContentBlock': undefined,
              'suggestFocusTopicsWithEmojiContentBlock': undefined,
              'reduceFocusTopicsContentBlock': undefined,
              'webSourcesContentBlock': undefined,
              'simpleRequestContentBlock': undefined,
            },
          ],
          'artifacts': undefined,
          'permissionChallenge': undefined,
          'isServerResult': false,
        },
        'inlineSearchEvent': undefined,
        'deepResearchEvent': undefined,
      },
    ],
    'createdTime': { 'internalValue': BigInt('13421965407189766') },
    'edits': undefined,
    'uploadedFiles': undefined,
    'skill': undefined,
    'fromBraveSearchSERP': false,
    'modelKey': undefined,
    'nearVerificationStatus': undefined,
  }),

  createConversationTurnWithDefaults({
    'uuid': '9145e4d6-d717-4a6e-b43a-63148f2bfaa6',
    'characterType': 1,
    'actionType': 1,
    'text':
      "Here's the breakdown of your mortgage:\n\n**Monthly Payment: $2,025.73**\n\n**Total Interest Over 25 Years: $257,717.54**\n\nWith your $150,000 down payment, you'd be borrowing $350,000. Over the 25-year term, you'll pay a total of $607,717.54 (your monthly payments combined), which means roughly 74% of what you borrow goes toward interest.\n\nA few things to keep in mind:\n- This assumes a fixed 4.9% rate for the full 25 years\n- This doesn't include property taxes, insurance, HOA fees, or maintenance costs, which will add to your monthly housing expenses\n- Early payments go mostly toward interest; principal paydown accelerates later in the loan\n- If you can afford higher payments, even small increases can significantly reduce total interest paid\n\nWould you like me to show you what happens if you adjust the down payment, loan term, or interest rate?",
    'prompt': undefined,
    'selectedText': undefined,
    'events': [
      {
        'completionEvent': {
          'completion':
            "Here's the breakdown of your mortgage:\n\n**Monthly Payment: $2,025.73**\n\n**Total Interest Over 25 Years: $257,717.54**\n\nWith your $150,000 down payment, you'd be borrowing $350,000. Over the 25-year term, you'll pay a total of $607,717.54 (your monthly payments combined), which means roughly 74% of what you borrow goes toward interest.\n\nA few things to keep in mind:\n- This assumes a fixed 4.9% rate for the full 25 years\n- This doesn't include property taxes, insurance, HOA fees, or maintenance costs, which will add to your monthly housing expenses\n- Early payments go mostly toward interest; principal paydown accelerates later in the loan\n- If you can afford higher payments, even small increases can significantly reduce total interest paid\n\nWould you like me to show you what happens if you adjust the down payment, loan term, or interest rate?",
        },
        'searchQueriesEvent': undefined,
        'searchStatusEvent': undefined,
        'conversationTitleEvent': undefined,
        'sourcesEvent': undefined,
        'contentReceiptEvent': undefined,
        'toolUseEvent': undefined,
        'inlineSearchEvent': undefined,
        'deepResearchEvent': undefined,
      },
    ],
    'createdTime': { 'internalValue': BigInt('13421965409430729') },
    'edits': undefined,
    'uploadedFiles': undefined,
    'skill': undefined,
    'fromBraveSearchSERP': false,
    'modelKey': undefined,
    'nearVerificationStatus': undefined,
  }),
]

export default history
