/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import braveComponentsStrings from 'gen/components/grit/brave_components_webui_mock_strings'

 // When building Storybook in transpileOnly mode `const enums` are not
 // preserved - to get around this we create a proxy which will return the
 // correct string key.
(globalThis as any).S = new Proxy({}, {
  get: (target, prop) => {
    return prop
  }
})

let locale: Record<string, string> = {
  ...braveComponentsStrings,
  about: 'about',
  accept: 'Accept',
  activityCopy:
    ' Brave Software. Brave is a registered trademark of Brave Software. Site names may be trademarks or registered trademarks of the site owner.',
  activityNote:
    'To protect your privacy, this Brave Rewards statement is not saved, recorded or logged anywhere other than on your device (this computer). It cannot be retrieved from Brave in the event of data loss on your device.',
  ads: 'Ads',
  ad: 'Ad',
  adsCurrentlyViewing: 'Your setting: maximum ads per hour: ',
  adsEarnings: 'earned from ads',
  adsHistoryFilterAll: 'All',
  adsHistoryFilterSaved: 'Saved',
  adsHistorySubTitle: "Ads you've received in the past {{totalDays}} days",
  adsHistoryTitle: 'Ads History',
  adsNotSupportedRegion: 'Sorry! Ads are not yet available in your region.',
  all: 'All',
  amount: 'Amount',
  and: 'and',
  autoContribute: 'Auto-Contribute',
  backupNow: 'Backup Now',
  backupWalletTitle: 'Backup Wallet',
  balanceUnavailable: 'Unavailable',
  bat: 'BAT',
  braveAdsDesc:
    'No action required. Just collect tokens. Your data is safe with our Shields.',
  braveAdsTitle: 'Brave Ads',
  braveAdsLaunchTitle: 'Brave Ads has arrived!',
  braveContributeDesc:
    'Set budget and browse normally. Your favorite sites get paid automatically.',
  braveContributeTitle: 'Auto-Contribute',
  braveRewards: 'Brave Rewards',
  braveRewardsDesc:
    'Earn tokens for viewing privacy-respecting ads, and pay it forward by supporting content creators you love!',
  braveRewardsOptInText: "Yes, I'm In!",
  braveRewardsSubTitle: 'Get Rewarded for Browsing!',
  braveRewardsTeaser: 'How it works...',
  braveRewardsTitle: 'Brave Rewards',
  braveVerified: 'Verified Creators',
  cancel: 'Cancel',
  category: 'Category',
  changeAmount: 'Change amount',
  click: 'Clicked',
  closeBalance: 'Closing Balance',
  contribute: 'Contribute',
  contributeAllocation: 'Auto-Contribute Allocation',
  date: 'Date',
  deposit: 'Deposit',
  deposits: 'Deposits',
  description: 'Description',
  details: 'Details',
  disabledPanelOff: 'Off',
  disabledPanelSettings: 'Settings.',
  disabledPanelPrivateText:
    'Brave Rewards is not available while in a Private Window.',
  disabledPanelText:
    'You are not currently accruing any Rewards benefits while browsing. Turn Rewards back on in',
  disabledPanelTextTwo:
    'Earn by viewing privacy-respecting ads, and pay it forward to support content creators you love.',
  disabledPanelTitle: 'Brave Rewards is',
  dismiss: 'Closed',
  dndCaptchaText1: 'Drag and drop the token logo onto the',
  dndCaptchaText2: 'target',
  donation: 'Donation',
  donationAmount: 'Donation amount',
  done: 'Done',
  downloadPDF: 'Download as PDF',
  earningsAds: 'Earnings from Ads',
  earningsViewDepositHistory: 'View deposit history',
  enableRewards: 'Enable Brave Rewards',
  excludeSite: 'Exclude this site',
  excludedSites: 'Sites Excluded',
  excludedSitesText: 'Sites excluded from Auto-Contributions:',
  expiresOn: 'expires on',
  for: 'for',
  import: 'import',
  includeInAuto: 'Include in Auto-Contribute',
  landed: 'Clicked',
  learnMore: 'Learn More',
  makeMonthly: 'Make this monthly',
  markAsInappropriate: 'Mark As Inappropriate',
  markAsInappropriateChecked: 'Mark As Inappropriate ✓',
  monthApr: 'Apr',
  monthAug: 'August',
  monthDec: 'December',
  monthFeb: 'February',
  monthJan: 'January',
  monthJul: 'July',
  monthJun: 'June',
  monthMar: 'March',
  monthMay: 'May',
  monthNov: 'November',
  monthOct: 'October',
  monthSep: 'September',
  monthlyText: 'Monthly',
  monthlyTip: 'Monthly Contribution',
  monthlyTips: 'Monthly Contributions',
  monthlyTipsBang: 'Monthly Contributions!',
  nextContribution: 'Next Contribution',
  noActivity: 'No activities yet…',
  noAdsHistory: 'There is currently no Brave Ads history',
  notEnoughTokens: 'Not enough tokens.',
  off: 'off',
  ok: 'ok',
  on: 'on',
  oneTimeDonation: 'Contributions',
  openAdsHistory: 'Show Ads History',
  openBalance: 'Opening Balance',
  payment: 'Payment',
  paymentNotMade: 'Payment not made.',
  pinnedSitesHeader: 'Pinned sites are now',
  pinnedSitesMsg: "Here's how monthly tips work:",
  pinnedSitesOne:
    "Monthly tips do not come out of your Auto-Contribute payment. They're separate.",
  pinnedSitesTwo: 'Each site is paid a fixed amount monthly.',
  pinnedSitesThree:
    "Monthly tips are paid out all at once, each month. If you're using Auto-Contribute, tips go out at the same time as your monthly Auto-Contribute payment.",
  pinnedSitesFour:
    'You can remove any site receiving monthly tips from inside of the Tips panel.',
  pleaseNote: 'Please note:',
  print: 'Print',
  privacyPolicy: 'Privacy Policy',
  processingRequest: 'You will be redirected shorty to verify your wallet.',
  readyToTakePart: 'Ready to get started?',
  readyToTakePartOptInText: "Yes I'm Ready!",
  readyToTakePartStart: 'You can start with the',
  reconnectWallet: 'Reconnect wallet',
  recurring: 'Recurring',
  recurringDonation: 'Recurring Donation',
  recurringDonations: 'Monthly Contributions',
  relaunch: 'Relaunch',
  remove: 'remove',
  resetWallet: 'Reset Brave Rewards',
  removeAdFromSaved: 'Remove From Saved',
  clearExcludeList: 'Clear exclude list',
  removeFromExcluded: 'Remove from excluded',
  reviewSitesMsg: 'Your pinned sites have been moved to',
  rewardsBannerText1:
    'Thanks for stopping by. We joined Brave’s vision of protecting your privacy because we believe that fans like you would support us in our effort to keep the web a clean and safe place to be.',
  rewardsContribute: 'Auto-Contribute',
  rewardsContributeAttention: 'Attention',
  rewardsExcludedText1: "You've excluded",
  rewardsExcludedText2: 'sites from Auto-Contribute.',
  rewardsSummary: 'Rewards Summary',
  rewardsWidgetBraveRewards: 'Brave Rewards',
  saveAd: 'Save',
  saved: 'Saved',
  seeAllItems: 'See all {{numItems}} items',
  seeAllSites: 'See all {{numSites}} sites',
  sendDonation: 'Send my donation',
  sendTip: 'Send my Contribution',
  serviceText:
    "By clicking 'Enable Brave Rewards', you indicate that you have read and agree to the",
  serviceTextToggle:
    'By turning on Brave Rewards, you indicate that you have read and agree to the',
  serviceTextPanelWelcome:
    'By clicking ‘Join Rewards’, you indicate that you have read and agree to the',
  serviceTextWelcome:
    "By clicking ‘Yes, I'm in!’, you indicate that you have read and agree to the",
  serviceTextReady:
    "By clicking ‘Yes, I'm Ready!’, you indicate that you have read and agree to the",
  set: 'Set...',
  setContribution: 'Set Contribution',
  settings: 'Settings',
  showAll: 'Show All',
  site: 'site',
  siteBannerNoticeNote: 'NOTE:',
  sites: 'sites',
  viewedSites: 'Sites Viewed',
  thankYou: 'Thank You!',
  termsOfService: 'Terms of Service',
  optOutTooltip: 'You will no longer receive\nads from this category',
  tipOnLike: 'Tip on like',
  titleBAT: 'Basic Attention token (BAT)',
  titleBTC: 'BitCoin (BTC)',
  titleETH: 'Ethereum (ETH)',
  titleLTC: 'Lite Coin (LTC)',
  tokens: 'tokens',
  total: 'Total',
  transactions: 'Transactions',
  turnOnAds: 'Turn on Ads',
  turnOnRewardsDesc:
    'This enables both Brave Ads and Auto-Contribute. You can always opt out each any time.',
  turnOnRewardsTitle: 'Turn on Rewards',
  tweetNow: 'Tweet',
  type: 'Type',
  uhOh: 'Uh oh…',
  unVerifiedCheck: 'Refresh status',
  unVerifiedPublisher: 'Unverified',
  unVerifiedText:
    'This creator has not yet signed up to receive contributions from Brave users.',
  unVerifiedTextMore: 'Learn more.',
  verifiedPublisher: 'Verified Creator',
  verifyWalletTitle: 'Verify your wallet',
  viewDetails: 'View Details',
  view: 'Viewed',
  viewMonthly: 'View Monthly Statement for Details',
  walletActivity: 'Wallet Activity',
  walletAddress: 'Wallet Address',
  walletBalance: 'wallet balance',
  walletButtonDisconnected: 'Logged out',
  walletButtonUnverified: 'Verify Wallet',
  walletButtonVerified: 'Verified',
  walletFailedButton: 'Re-try',
  walletFailedTitle: 'Wallet creation failed',
  walletFailedText: 'Please check your internet connection.',
  walletGoToVerifyPage: 'Complete wallet verification',
  walletGoToUphold: 'Go to your Uphold account',
  walletDisconnect: 'Log out from Brave Rewards',
  walletVerificationButton: 'Verify wallet',
  walletVerificationFooter: 'Our wallet service is provided by Uphold',
  walletVerificationList1:
    'Withdraw BAT that you earn from viewing privacy-respecting ads',
  walletVerificationList2:
    'Purchase additional BAT with credit cards and other sources',
  walletVerificationList3:
    'Withdraw BAT that you may have previously added to your Brave Rewards wallet',
  walletVerificationListHeader: 'Benefits of verifying',
  walletVerificationNote1:
    'Uphold may require you to verify your identity based on services requested.',
  walletVerificationNote2:
    'Brave Software Inc. does not process, store, or access any of the personal information that you provide to Uphold when you establish an account with them.',
  walletVerificationTitle1: 'Ready to verify your wallet?',
  walletVerified: 'Verified',
  welcome: 'Welcome!',
  welcomeBack: 'Welcome Back!',
  welcomeButtonTextOne: 'Start Earning Now!',
  welcomeButtonTextTwo: 'Join Rewards',
  welcomeDescOne:
    'You can now earn tokens for watching privacy focused Brave Ads. Your contribution stays the same.',
  welcomeDescTwo:
    'Earn tokens for watching Ads and pay it forward to your favorite content creators.',
  welcomeFooterTextOne: 'Check out what’s improved',
  welcomeFooterTextTwo: 'Learn More',
  welcomeHeaderOne: 'Brave Payments is now Brave Rewards with many upgrades.',
  welcomeHeaderTwo:
    'You are about to start a very Brave way to browse the web.',
  whyBraveRewards: 'Why Brave Rewards?',
  whyBraveRewardsDesc1:
    'With conventional browsers, you pay to browse the web by viewing ads with your valuable attention, spending your valuable time downloading invasive ad technology, that transmits your valuable private data to advertisers — without your consent.',
  whyBraveRewardsDesc2:
    "Well, you've come to the right place. Brave welcomes you to the new internet. One where your time is valued, your personal data is kept private, and you actually get paid for your attention.",
  whyHow: 'Why & How',
  yourWallet: 'Your wallet',
  // New Tab Page
  adsTrackersBlocked: 'Ads and Trackers Blocked',
  estimatedBandwidthSaved: 'Bandwidth saved',
  estimatedTimeSaved: 'Estimated Time Saved',
  minutes: 'minutes',
  photoBy: 'Photo by',
  customize: 'Customize',
  thumbRemoved: 'Top site removed',
  undoRemoved: 'Undo',
  close: 'Close',
  hide: 'Hide',
  dashboardSettingsTitle: 'Customize Dashboard',
  showBackgroundImg: 'Show background image',
  showBraveStats: 'Show Brave Stats',
  braveTalkWidgetTitle: 'Brave Talk',
  showClock: 'Show Clock',
  showTopSites: 'Show Top Sites',
  showRewards: 'Show Rewards',
  tosAndPp:
    'By turning on {{title}}, you agree to the $1Terms of Service$2 and $3Privacy Policy$4.',
  editCardsTitle: 'Edit Cards',
}

export function provideStrings(strings: Record<string, string>) {
  locale = {
    ...locale,
    ...strings
  }
}

export function getString(key: string): string {
  return locale[key] || key
}
