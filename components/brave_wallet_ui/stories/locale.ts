// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { provideStrings } from '../../../.storybook/locale'

provideStrings({
  // Entities
  braveWalletContract: 'Contract',

  // Search Text
  braveWalletSearchText: 'Search',

  // Side Nav Buttons
  braveWalletSideNavCrypto: 'Crypto',
  braveWalletSideNavRewards: 'Rewards',
  braveWalletSideNavCards: 'Cards',

  // Top Nav Tab Buttons
  braveWalletTopNavPortfolio: 'Portfolio',
  braveWalletTopTabPrices: 'Prices',
  braveWalletTopTabApps: 'Apps',
  braveWalletTopNavNFTS: 'NFTs',
  braveWalletTopNavAccounts: 'Accounts',
  braveWalletTopNavMarket: 'Market',

  // Chart Timeline Buttons
  braveWalletChartOneHour: '1 Hour',
  braveWalletChartOneDay: '1 Day',
  braveWalletChartOneWeek: '1 Week',
  braveWalletChartOneMonth: '1 Month',
  braveWalletChartThreeMonths: '3 Months',
  braveWalletChartOneYear: '1 Year',
  braveWalletChartAllTime: 'All time',
  braveWalletChartOneHourAbbr: '1H',
  braveWalletChartOneDayAbbr: '1D',
  braveWalletChartOneWeekAbbr: '1W',
  braveWalletChartOneMonthAbbr: '1M',
  braveWalletChartThreeMonthsAbbr: '3M',
  braveWalletChartOneYearAbbr: '1Y',
  braveWalletChartAllTimeAbbr: 'All',

  // Portfolio View
  braveWalletAddCoin: 'Add Coin',
  braveWalletBalance: 'Balance',
  braveWalletPortfolioAssetNetworkDescription: '$1 on $2',

  // Portfolio SubView
  braveWalletAccounts: 'Accounts',
  braveWalletSubviewAccount: 'Account',
  braveWalletOwner: 'Owner',
  braveWalletActivity: 'Activity',
  braveWalletTransactions: 'Transactions',
  braveWalletPrice: 'Price',
  braveWalletBack: 'Back',
  braveWalletAddAccount: 'Add Account',
  braveWalletPoweredByCoinGecko: 'Price data powered by CoinGecko',

  // Actions
  braveWalletClickToSwitch: 'Click to switch',
  braveWalletEnterYourPassword: 'Enter your password',
  braveWalletEnterAPasswordToContinue: 'Enter a password to continue',
  braveWalletEnterYourPasswordToStartBackup:
    'Enter your Brave Wallet password to start backing up wallet.',

  // BuySendSwap
  braveWalletBuy: 'Buy',
  braveWalletSend: 'Send',
  braveWalletSwap: 'Swap',
  braveWalletBridge: 'Bridge',
  braveWalletReset: 'Reset',
  braveWalletSell: 'Sell',
  braveWalletNotEnoughBalance: 'You don’t have enough $1 in this account.',
  braveWalletSlippageToleranceWarning: 'Transaction may be frontrun',
  braveWalletExpiresInTitle: 'Expires in',
  braveWalletSendPlaceholder: 'Wallet address or URL',
  braveWalletSendNoURLPlaceholder: 'Wallet address',
  braveWalletSwapDisclaimer: 'Brave uses $1$3$2 as a DEX aggregator.',
  braveWalletSwapDisclaimerDescription:
    '0x will process the Ethereum address and IP address ' +
    'to fulfill a transaction (including getting quotes). ' +
    '0x will ONLY use this data for the purposes of processing transactions.',
  braveWalletJupiterSwapDisclaimerDescription:
    'Jupiter will process the Solana address and IP address ' +
    'to fulfill a transaction (including getting quotes). ' +
    'Jupiter will ONLY use this data ' +
    'for the purposes of processing transactions.',
  braveWalletSwapFeesNotice: 'Quote includes a $1 Brave fee.',
  braveWalletDecimalPlacesError: 'Too many decimal places',
  braveWalletBuyTapBuyNotSupportedMessage:
    'Buy not supported for selected network',
  braveWalletSearchingForDomain: 'Searching for domain...',
  braveWalletEnsOffChainLookupTitle:
    'Brave supports using off-chain gateways to resolve .eth domains.',
  braveWalletEnsOffChainLookupDescription:
    "It looks like you've entered an ENS address. " +
    "We'll need to use a third-party resolver to resolve this request, " +
    'which may be able to see your IP address and domain.',
  braveWalletEnsOffChainButton: 'Use ENS domain',
  braveWalletFEVMAddressTranslationTitle:
    'ETH address will be converted to the Filecoin address.',
  braveWalletFEVMAddressTranslationDescription:
    "It looks like you've entered an ENS address. " +
    "We'll need to use a third-party resolver to resolve this request, " +
    'which may be able to see your IP address and domain.',
  braveWalletFetchingQuote: 'Fetching quote',
  braveWalletReceiveEstimate: 'Receive (est.)',
  braveWalletFromToken: 'From token',
  braveWalletToToken: 'To token',
  braveWalletNewQuoteIn: 'New quote in $1$3$2',

  // Send Tab
  braveWalletChooseAssetToSend: 'Choose asset to send',
  braveWalletChooseAssetToSwap: 'Choose asset to swap',
  braveWalletEnterRecipientAddress: 'Enter recipient address',
  braveWalletNotEnoughFunds: 'Not enough funds',
  braveWalletSendHalf: 'HALF',
  braveWalletSendMax: 'MAX',
  braveWalletReviewSend: 'Review send',
  braveWalletNoAvailableTokens: 'No available tokens',
  braveWalletSearchTokens: 'Search by name or paste address',
  braveWalletSearchNFTs: 'Search NFT by name, id',
  braveWalletChooseAsset: 'Choose asset',
  braveWalletOrdinalsWarningMessage:
    'Brave Wallet does not currently support Bitcoin NFTs (ordinals). Sending BTC from an address that has ordinals may result in its ordinals being transferred inadvertently.',
  braveWalletUserUnderstandsLabel: 'I understand',
  braveWalletChooseRecipient: 'Choose recipient',
  braveWalletMyAddresses: 'My addresses',
  braveWalletAddressOrDomainPlaceholder: 'Enter public address or domain.',
  braveWalletSendTo: 'Send to',
  braveWalletInvalidDomainExtension: 'Invalid domain extension.',
  braveWalletChooseAccount: 'Choose an account to continue',
  braveWalletMarketPrice: 'Market price',
  braveWalletOwned: 'Owned',
  braveWalletNotOwned: 'Not owned',
  braveWalletAmount24H: 'Amount/24h',

  // Bridge
  braveWalletChooseAssetToBridge: 'Choose asset to bridge',
  braveWalletEstTime: 'Est time',
  braveWalletExchangeRate: 'Exchange rate',
  braveWalletExchangeFor: '$1 for $2',
  braveWalletRecipient: 'Recipient',
  braveWalletReviewBridge: 'Review bridge',

  // Create Account Tab
  braveWalletCreateAccountDescription:
    'You don’t yet have a $1 account. Create one now?',
  braveWalletCreateAccountYes: 'Yes',
  braveWalletCreateAccountNo: 'No',

  // Buttons
  braveWalletButtonContinue: 'Continue',
  braveWalletContinueWithXItems: 'Continue with $1 $2',
  braveWalletButtonNext: 'Next',
  braveWalletButtonVerifyPhrase: 'Show my recovery phrase',
  braveWalletButtonCopy: 'Copy',
  braveWalletButtonCopied: 'Copied!',
  braveWalletButtonVerify: 'Verify',
  braveWalletButtonClose: 'Close',
  braveWalletButtonMore: 'More',
  braveWalletButtonDone: 'Done',
  braveWalletButtonSkip: 'Skip',
  braveWalletButtonCancel: 'Cancel',
  braveWalletButtonSaveChanges: 'Save changes',
  braveWalletLearnMore: 'Learn more',
  braveWalletButtonNoThanks: 'No thanks',
  braveWalletButtonEnable: 'Enable',
  braveWalletProceedButton: 'Proceed',

  // Onboarding - Welcome
  braveWalletWelcomeRestoreButton: 'Restore',
  braveWalletWelcomeTitle: 'Browser-native. Self-custody. And multi-chain.',
  braveWalletWelcomeDescription:
    'Take control of your crypto and NFTs. ' +
    'Brave Wallet supports Ethereum, EVM chains, ' +
    'Solana, Filecoin, Bitcoin, and more.',
  braveWalletWelcomeNewWalletTitle: 'Need a new wallet?',
  braveWalletWelcomeNewWalletDescription:
    'Get started with Brave Wallet in minutes.',
  braveWalletWelcomeImportWalletTitle: 'Already have a wallet?',
  braveWalletWelcomeImportWalletDescription: 'Import your existing wallet.',
  braveWalletWelcomeWatchOnlyTitle: 'I need a watch-only wallet',
  braveWalletCopyright:
    '©2024 Brave Software Inc. Brave and the Brave logo are registered ' +
    'trademarks of Brave Software, Inc. Other product names and logos may be ' +
    'trademarks of their respective companies. All rights reserved.',
  braveWalletWelcomeDividerText: 'or',

  // Onboarding - Disclosures
  braveWalletDisclosuresTitle: 'Before we begin',
  braveWalletDisclosuresDescription:
    'We require that you acknowledge the items below',
  braveWalletSelfCustodyDisclosureCheckboxText:
    'I understand that this is a self-custody wallet, ' +
    'and that I am solely responsible for any associated funds, assets, ' +
    'or accounts, and for taking any appropriate action to ' +
    'secure, protect, and back up my wallet. ' +
    'I understand that Brave cannot access my wallet or ' +
    'reverse transactions on my behalf, ' +
    'and that my recovery phrase is the ONLY way to ' +
    'regain access in the event of a ' +
    'lost password, stolen device, or similar circumstance.',
  braveWalletTermsOfServiceCheckboxText:
    'I have read and agree to the $1Terms of use$2.',

  // Onboarding import or restore wallet page
  braveWalletCheckingInstalledExtensions: 'Checking for wallet extensions...',
  braveWalletImportOrRestoreWalletTitle: 'Connect to your existing wallet',
  braveWalletImportOrRestoreDescription:
    'To connect a wallet you already have, ' +
    'you may need to enter your recovery phrase. At this time we support ' +
    'restoring / importing from Ethereum and Solana wallets.',
  braveWalletRestoreMyBraveWallet: 'Import an existing wallet',
  braveWalletRestoreMyBraveWalletDescription: '12-24 words',
  braveWalletImportFromMetaMask: 'Import from MetaMask',
  braveWalletImportFromMetaMaskDescription:
    'Use your MetaMask password to import your seed phrase',
  braveWalletImportFromLegacy: 'Import from legacy Brave crypto wallets',
  braveWalletCreateWalletInsteadLink: 'Never mind, I’ll create a new wallet',
  braveWalletRestoreAlternateLength: 'I have a $1-word recovery phrase',
  braveWalletRestoreWalletError: 'Check your recovery phrase and try again.',
  braveWalletImportWithRecoveryPhrase: 'Import using your recovery phrase',
  braveWalletMetamaskDetected: 'Metamask wallet detected!',
  braveWalletLegacyWalletDetected: 'Legacy wallet detected!',
  braveWalletUsePassword: 'Use password',
  braveWalletRecoveryPhraseWord: 'Word $1',

  // onboarding import wallet screen
  braveWalletImportPasswordError: 'Password is not correct',
  braveWalletMetaMaskPasswordInputPlaceholder: 'Type MetaMask password',
  braveWalletImportFromMetaMaskSeedInstructions:
    'Type your MetaMask 12-24 word recovery phrase.',
  braveWalletMetaMaskExtensionDetected: 'Import MetaMask wallet',

  braveWalletMetaMaskExtensionImportDescription:
    'You can easily import your MetaMask wallet just by inputting ' +
    'your password.',
  braveWalletRestoreMyBraveWalletInstructions:
    'You can paste your entire recovery phrase into any field.',
  braveWalletRecoveryPhraseLengthError:
    'Recovery phrase must be 12, 15, 18, 21, or 24 words long',
  braveWalletInvalidMnemonicError:
    'The mnemonic being imported is not valid for Brave Wallet',
  braveWalletImportWalletTypeTitle:
    'Which type of wallet would you like to import?',
  braveWalletImportWalletTypeHotWalletTitle: 'Ethereum/Solana/Filecoin wallet',
  braveWalletImportWalletTypeHotWalletDescription:
    'Import your seed phrase from an existing wallet',
  braveWalletImportWalletTypeHardwareWalletTitle: 'Hardware wallet',
  braveWalletImportWalletTypeHardwareWalletDescription:
    'Connect your hardware wallet with Brave',

  // Onboarding - Backup Wallet - Intro
  braveWalletOnboardingRecoveryPhraseBackupIntroTitle:
    'Save your recovery phrase',
  braveWalletOnboardingRecoveryPhraseBackupIntroDescription:
    'Your recovery phrase is the key to access your wallet in case you ' +
    'forget your password or lose your device.',
  braveWalletRecoveryPhraseBackupWarningImportant:
    'Keep it in a secure place that is not accessible to others and avoid ' +
    'sharing it with anyone.',
  braveWalletRecoveryWordInstructions:
    'Enter the word in position $1 from your recovery phrase.',
  braveWalletVerifyRecoveryPhraseGoBack: 'Forgot to save? Go back',

  // Onboarding - Backup Wallet - Recovery Phrase Backup
  braveWalletRecoveryPhraseBackupTitle: 'Back up your wallet recovery phrase',
  braveWalletRecoveryPhraseBackupWarning:
    'Brave cannot access your secret recovery phrase. ' +
    'Keep it safe, and never share it with anyone else.',
  braveWalletCopiedToClipboard: 'Copied to clipboard',
  braveWalletClickToSeeRecoveryPhrase: 'Click to see your phrase',
  braveWalletSkipBackupWarning:
    'You can skip this step now, but you still need to back up your recovery ' +
    'phrase to ensure account security.',
  braveWalletSkipBackup: 'I’ll back up later',
  braveWalletGoBack: 'Go back',

  // Onboarding - Backup Wallet - Verify Recovery Phrase
  braveWalletVerifyRecoveryPhraseTitle: "Let's check",
  braveWalletVerifyRecoveryPhraseInstructions:
    'Click the $1$7 ($8)$2, $3$9 ($10)$4, and $5$11 ($12)$6 words of ' +
    'your recovery phrase.',
  braveWalletVerifyPhraseError: "Recovery phrase didn't match",

  // Recovery Phrase Backup - Intro
  braveWalletBackupIntroTitle: 'Back up your crypto wallet',
  braveWalletBackupIntroTerms:
    'I understand that if I lose my recovery words, ' +
    'I will not be able to access my crypto wallet.',

  // Recovery Phrase Backup - Intro
  braveWalletRecoveryTitle: 'Your recovery phrase',
  braveWalletRecoveryDescription:
    'Write down or copy these words in the exact order shown below, ' +
    'and save them somewhere safe. Your recovery phrase is the only ' +
    'way to regain account access in case of forgotten password, ' +
    'lost or stolen device, or you want to switch wallets.',
  braveWalletRecoveryWarning1: 'WARNING:',
  braveWalletRecoveryWarning2: 'Never share your recovery phrase.',
  braveWalletRecoveryWarning3:
    'Anyone with this phrase can take your assets forever.',
  braveWalletRecoveryTerms: 'I have backed up my phrase somewhere safe.',

  // Recovery Phrase Backup - Verify Recovery Phrase
  braveWalletVerifyRecoveryTitle: 'Verify recovery phrase',
  braveWalletVerifyRecoveryDescription:
    'Select the words in your recovery phrase in their correct order.',
  braveWalletVerifyError: 'Recovery phrase did not match, please try again.',

  // Create Password
  braveWalletCreatePasswordTitle: 'Create a new password',
  braveWalletCreatePasswordDescription:
    "You'll use this password to access your wallet.",
  braveWalletCreatePasswordInput: 'Enter new password',
  braveWalletConfirmPasswordInput: 'Re-enter password',
  braveWalletCreatePasswordError: "Password criteria doesn't match.",
  braveWalletConfirmPasswordError: 'Passwords do not match',
  braveWalletPasswordMatch: 'Passwords match',
  braveWalletPasswordIsStrong: 'Strong!',
  braveWalletPasswordIsMediumStrength: 'Medium',
  braveWalletPasswordIsWeak: 'Weak',
  braveWalletAutoLockTitle: 'Brave Wallet will auto-lock after',
  braveWalletAutoLockDurationMinutes: '$1 minutes',
  braveWalletAutoLockDurationHours: '$1 hour(s)',

  // Creating Wallet
  braveWalletCreatingWallet: 'Creating Wallet...',

  // Create Password - Strength Tooltip
  braveWalletPasswordStrengthTooltipHeading: 'Password should have:',
  braveWalletPasswordStrengthTooltipIsLongEnough: 'At least 8 characters',

  // Onboarding Success
  braveWalletOnboardingSuccessTitle: 'You’re all set!',
  braveWalletOnboardingSuccessDescription:
    'Your Brave Wallet is now set up and ready for use.',
  braveWalletBuyCryptoButton: 'Buy crypto',
  braveWalletDepositCryptoButton: 'Deposit',
  braveWalletLearnAboutMyWallet: 'Learn more about my new wallet',
  braveWalletOnboardingSuccessGoToPortfolio: 'Go to portfolio',

  // Wallet Article Links
  braveWalletArticleLinkWhatsARecoveryPhrase: 'What’s a recovery phrase?',

  // Lock Screen
  braveWalletEnterYourBraveWalletPassword: 'Enter your Brave Wallet password',
  braveWalletLockScreenTitle: 'Enter password to unlock wallet',
  braveWalletLockScreenButton: 'Unlock',
  braveWalletLockScreenError: 'Incorrect password',
  braveWalletUnlockWallet: 'Unlock Wallet',

  // Wallet More Popup
  braveWalletWalletPopupSettings: 'Settings',
  braveWalletWalletPopupLock: 'Lock wallet',
  braveWalletWalletPopupBackup: 'Backup Wallet',
  braveWalletWalletPopupConnectedSites: 'Connected sites',
  braveWalletWalletPopupHideBalances: 'Balances',
  braveWalletWalletPopupShowGraph: 'Graph',
  braveWalletWalletNFTsTab: 'NFTs tab',

  // Backup Warning
  braveWalletBackupWarningText:
    'Back up your wallet now ' +
    'to protect your assets and ensure you never lose access.',
  braveWalletBackupButton: 'Back up now',
  braveWalletDismissButton: 'Dismiss',

  // Default Wallet Banner
  braveWalletDefaultWalletBanner:
    'Brave Wallet is not set as your default wallet and will not ' +
    'respond to Web3 DApps. Visit settings to change your default wallet.',

  // Restore Screen
  braveWalletRestoreTite: 'Restore primary crypto accounts',
  braveWalletRestoreDescription:
    'Enter your recovery phrase to restore your Brave wallet crypto account.',
  braveWalletRestoreError: 'The recovery phrase entered is invalid.',
  braveWalletRestorePlaceholder: 'Paste recovery phrase from clipboard',
  braveWalletRestoreShowPhrase: 'Show recovery phrase',
  braveWalletRestoreLegacyCheckBox: 'Import from legacy Brave crypto wallets?',
  braveWalletRestoreFormText: 'New Password',

  // Clipboard
  braveWalletToolTipCopyToClipboard: 'Copy to Clipboard',
  braveWalletToolTipCopiedToClipboard: 'Copied',
  braveWalletPasteFromClipboard: 'Paste from clipboard',
  braveWalletClickToCopy: 'Click to copy',

  // Accounts Tab
  braveWalletAccountsPrimary: 'Primary crypto accounts',
  braveWalletAccountsSecondary: 'Imported accounts',
  braveWalletConnectedHardwareWallets: 'Connected hardware wallets',
  braveWalletAccountsAssets: 'Assets',
  braveWalletAccountsEditVisibleAssets: 'Visible assets',
  braveWalletAccountBalance: 'Account balance',
  braveWalletViewAddressOn: 'View address on:',
  braveWalletNetworkExplorer: '$1 Explorer',

  // Add Account Options
  braveWalletCreateAccount: 'Create $1 account',
  braveWalletAddAccountCreate: 'Create',
  braveWalletAddAccountImport: 'Import',
  braveWalletAddAccountImportHardware: 'Import from hardware wallet',
  braveWalletAddAccountHardware: 'Hardware',
  braveWalletAddAccountConnect: 'Connect',
  braveWalletAddAccountPlaceholder: 'Name this account',
  braveWalletCreateAccountButton: 'Create account',
  braveWalletCreateAccountImportAccount: 'Import $1 account',
  braveWalletCreateAccountTitle: 'Select one of the following account types',
  braveWalletCreateAccountEthereumDescription:
    'Supports EVM compatible assets on the Ethereum blockchain ' +
    '(ERC-20, ERC-721, ERC-1551, ERC-1155)',
  braveWalletCreateAccountSolanaDescription:
    'Supports SPL compatible assets on the Solana blockchain',
  braveWalletCreateAccountBitcoinDescription: 'Store BTC assets',
  braveWalletCreateAccountZCashDescription: 'Store ZEC assets',
  braveWalletCreateAccountFilecoinDescription: 'Store FIL asset',
  braveWalletFilecoinPrivateKeyProtocol: 'Private key $1',
  braveWalletEnterPasswordIfApplicable: 'Enter password (if applicable)',
  braveWalletPrivateKeyImportType: 'Import type',
  braveWalletFilImportPrivateKeyFormatDescription:
    'NOTE: Private key must be provided as a hex-encoded JSON that contains ' +
    'private key and protocol information. $1Learn more$2.',

  // Import Account
  braveWalletImportAccountDisclaimer:
    'These accounts can be used with Web3 DApps, and can be shown in your ' +
    'portfolio. However, note that secondary accounts cannot be restored ' +
    'via recovery phrase from your primary account backup.',
  braveWalletImportAccountPlaceholder: 'Paste private key from clipboard',
  braveWalletImportAccountKey: 'Private key',
  braveWalletImportAccountFile: 'JSON file',
  braveWalletImportAccountUploadButton: 'Choose file',
  braveWalletImportAccountUploadPlaceholder: 'No file chosen',
  braveWalletImportAccountError: 'Failed to import account, please try again.',
  braveWalletImportAccount: 'Import account',

  // Connect Hardware Wallet
  braveWalletConnectHardwareTitle: 'Select your hardware wallet device',
  braveWalletConnectHardwareInfo1:
    'Connect your $1 wallet directly to your computer.',
  braveWalletConnectHardwareInfo2: 'Unlock your device and select the $1 app.',
  braveWalletConnectHardwareTrezor: 'Trezor',
  braveWalletConnectHardwareLedger: 'Ledger',
  braveWalletConnectHardwareDeviceDescription:
    'Connect your $1 device to Brave Wallet',
  braveWalletConnectHardwareAuthorizationNeeded:
    'Grant Brave access to your $1 device.',
  braveWalletConnectingHardwareWallet: 'Connecting...',
  braveWalletAddCheckedAccountsHardwareWallet: 'Add checked accounts',
  braveWalletLoadMoreAccountsHardwareWallet: 'Load more',
  braveWalletLoadingMoreAccountsHardwareWallet: 'Loading more...',
  braveWalletSearchScannedAccounts: 'Search account',
  braveWalletHDPath: 'HD path',
  braveWalletSwitchHDPathTextHardwareWallet:
    'Try switching HD path (above) if you ' +
    'cannot find the account you are looking for.',
  braveWalletLedgerLiveDerivationPath: 'Ledger Live',
  braveWalletLedgerLegacyDerivationPath: 'Legacy (MEW/MyCrypto)',
  braveWalletUnknownInternalError:
    'Unknown error, please reconnect your hardware wallet and try again.',
  braveWalletConnectHardwareSearchNothingFound: 'No results found.',
  braveWalletAuthorizeHardwareWallet: 'Authorize your wallet',
  braveWalletConnectHardwareWallet: 'Connect hardware wallet',
  braveWallectConnectHardwareDescription:
    'We currently support $1 and $2 devices.',
  braveWalletImportHardwareWalletDescription:
    'Connect your hardware wallet to manage your assets directly ' +
    'from Brave Wallet',
  braveWalletConnectHardwareWalletSelectBlockchain:
    'Select a blockchain to import your hardware wallet',

  // Account Settings Modal
  braveWalletAccountSettingsWatchlist: 'Visible assets',
  braveWalletAccountSettingsPrivateKey: 'Private key',
  braveWalletAccountSettingsSave: 'Save',
  braveWalletAccountSettingsRemove: 'Remove account',
  braveWalletWatchlistAddCustomAsset: 'Add custom asset',
  braveWalletWatchListTokenName: 'Token name',
  braveWalletWatchListTokenAddress: 'Token address',
  braveWalletWatchListTokenSymbol: 'Token symbol',
  braveWalletWatchListTokenDecimals: 'Decimals of precision',
  braveWalletWatchListAdd: 'Add',
  braveWalletWatchListDoneButton: 'Done',
  braveWalletWatchListNoAsset: 'No assets named',
  braveWalletWatchListSearchPlaceholder: 'Search assets or contract address',
  braveWalletWatchListError: 'Failed to add custom token, please try again.',
  braveWalletCustomTokenExistsError:
    'This token has already been added to your portfolio.',
  braveWalletAccountSettingsDisclaimer:
    'WARNING: Never share your private key. ' +
    'Anyone with this private key can take your assets forever.',
  braveWalletAccountSettingsShowKey: 'Show key',
  braveWalletAccountSettingsHideKey: 'Hide key',
  braveWalletAccountSettingsUpdateError:
    'Failed to update account name, please try again.',
  braveWalletWatchListTokenId: 'Token ID (only for ERC721)',
  braveWalletWatchListTokenIdError: 'Token ID is required',
  braveWalletWatchListAdvanced: 'Advanced',
  braveWalletWatchListCoingeckoId: 'Coingecko ID',
  braveWalletIconURL: 'Icon URL',
  braveWalletAddAsset: 'Add asset',
  braveWalletAccountsExport: 'Export',
  braveWalletAccountsDeposit: 'Deposit',
  braveWalletAccountsRemove: 'Remove',
  braveWalletAccountSettings: 'Account settings',

  // Empty Token List State
  braveWalletNoAvailableAssets: 'No available assets',
  braveWalletNoAvailableAssetsDescription:
    "Deposit or purchase tokens to get started. If you don't see tokens " +
    'from an imported account, check the filters and display settings. ' +
    'Unknown tokens may need to be added as custom assets.',

  // AmountPresets
  braveWalletPreset25: '25%',
  braveWalletPreset50: '50%',
  braveWalletPreset75: '75%',
  braveWalletPreset100: '100%',

  // Ordinals
  braveWalletOrdinalFirst: 'First',
  braveWalletOrdinalSecond: 'Second',
  braveWalletOrdinalThird: 'Third',
  braveWalletOrdinalFourth: 'Fourth',
  braveWalletOrdinalFifth: 'Fifth',
  braveWalletOrdinalSixth: 'Sixth',
  braveWalletOrdinalSeventh: 'Seventh',
  braveWalletOrdinalEighth: 'Eighth',
  braveWalletOrdinalNinth: 'Ninth',
  braveWalletOrdinalTenth: 'Tenth',
  braveWalletOrdinalEleventh: 'Eleventh',
  braveWalletOrdinalTwelfth: 'Twelfth',
  braveWalletOrdinalThirteenth: 'Thirteenth',
  braveWalletOrdinalFourteenth: 'Fourteenth',
  braveWalletOrdinalFifteenth: 'Fifteenth',
  braveWalletOrdinalSixteenth: 'Sixteenth',
  braveWalletOrdinalSeventeenth: 'Seventeenth',
  braveWalletOrdinalEighteenth: 'Eighteenth',
  braveWalletOrdinalNineteenth: 'Nineteenth',
  braveWalletOrdinalTwentieth: 'Twentieth',
  braveWalletOrdinalTwentyFirst: 'Twenty-first',
  braveWalletOrdinalTwentySecond: 'Twenty-second',
  braveWalletOrdinalTwentyThird: 'Twenty-third',
  braveWalletOrdinalTwentyFourth: 'Twenty-fourth',
  braveWalletOrdinalSuffixOne: 'st',
  braveWalletOrdinalSuffixTwo: 'nd',
  braveWalletOrdinalSuffixFew: 'rd',
  braveWalletOrdinalSuffixOther: 'th',

  // Networks
  braveWalletNetworkETH: 'Ethereum',
  braveWalletNetworkMain: 'Mainnet',
  braveWalletNetworkTest: 'Test Network',
  braveWalletNetworkLocalhost: 'Localhost',
  braveWalletAddNetworksAnytimeInSettings:
    'You can add networks anytime in Settings.',
  braveWalletSupportedNetworks: 'Supported networks',
  braveWalletChooseChainsToUse:
    'Choose which blockchains to use in your wallet.',
  braveWalletShowTestnets: 'Show testnets',

  // Grouping Labels
  braveWalletFeatured: 'Featured',
  braveWalletPopular: 'Popular',

  // Select Screens
  braveWalletSelectAccount: 'Select account',
  braveWalletSearchAccount: 'Search accounts',
  braveWalletSelectNetwork: 'Select network',
  braveWalletSelectAsset: 'Select from',
  braveWalletSearchAsset: 'Search coins',
  braveWalletSelectCurrency: 'Select currency',
  braveWalletSearchCurrency: 'Search currencies',

  // Swap
  braveWalletSwapFrom: 'Amount',
  braveWalletSwapTo: 'To',
  braveWalletSwapEstimate: 'estimate',
  braveWalletSwapMarket: 'Market',
  braveWalletSwapLimit: 'Limit',
  braveWalletSwapPriceIn: 'Price in',
  braveSwapInsufficientBalance: 'Insufficient balance',
  braveWalletSwapInsufficientFundsForGas: 'Insufficient funds for gas',
  braveSwapApproveToken: 'Approve $1',
  braveSwapInsufficientLiquidity: 'Insufficient liquidity',
  braveWalletSwapInsufficientAllowance: 'Activate token',
  braveWalletSwapUnknownError: 'Unknown error',
  braveWalletSwapReviewSpend: 'You spend',
  braveWalletSwapReviewReceive: "You'll receive",
  braveWalletSwapReviewHeader: 'Confirm order',
  braveWalletSolanaSwap: 'Solana Swap',
  braveWalletNoRoutesFound: 'No routes found',
  braveWalletReviewSwap: 'Review swap',
  braveWalletChooseQuoteProvider: 'Choose quote provider',
  braveWalletQuoteProviderInfo:
    'Each aggregator provides quotes by using liquidity from its API. ' +
    'They gather price information from various exchanges and offer the ' +
    'prices based on this data.',
  braveWalletNotAvailable: 'Not available',
  braveWalletUpdate: 'Update',
  braveWalletProviderNotSupported: 'This pair is not supported by $1',
  braveWalletChangeProvider: 'Change provider',
  braveWalletRoute: 'Route',
  braveWalletMaxSlippage: 'Max slippage',
  braveWalletMaxSlippageDescription:
    'When the deviation between the price of the transaction you ' +
    'submitted and the price at the time of the transaction on chain is ' +
    'greater than this set value, the transaction will fail.',
  braveWalletPriceImpactDescription:
    'Price impact is how much your trade might move the market price. ' +
    'Some tokens with low liquidity are more sensitive to trades, so even ' +
    'smaller trades can shift the price a lot.',
  braveWalletSuggestedValues: 'Suggested Values:',
  braveWalletMainstreamAssetPairs: 'Mainstream Asset Pairs: $1%',
  braveWalletStablecoinPairs: 'Stablecoin Pairs: $1%',
  braveWalletOthers: 'Others: $1%',
  braveWalletSuggested: 'Suggested',
  braveWalletCustom: 'Custom',
  braveWalletCheapest: 'Cheapest',
  braveWalletFastest: 'Fastest',
  braveWalletOnNetwork: 'on $1',
  braveWalletRoutes: 'Routes',
  braveWalletSwapOnNetworkViaExchange: 'Swap on $1 via $2',
  braveWalletBridgeFromNetworkToNetwork: 'Bridge from $1 to $2',
  braveWalletExchangeNamePlusSteps: '$1 + $2 $3',
  braveWalletExchangeViaProvider: '$1$5$2 via $3$6$4',
  braveWalletSortBy: 'Sort by',

  // Buy
  braveWalletBuyTitle: 'Test faucet',
  braveWalletBuyDescription: 'Get Ether from a faucet for $1',
  braveWalletBuyFaucetButton: 'Get Ether',
  braveWalletBuyContinueButton: 'Select purchase method',
  braveWalletBuySelectAsset: 'Select an asset',
  braveWalletBuyRampNetworkName: 'Ramp.Network',
  braveWalletBuySardineName: 'Sardine',
  braveWalletBuyTransakName: 'Transak',
  braveWalletBuyStripeName: 'Link by Stripe',
  braveWalletBuyCoinbaseName: 'Coinbase Pay',
  braveWalletBuyRampDescription:
    'Buy with CC/Debit or ACH. ' + 'Competitive Rates.',
  braveWalletBuySardineDescription:
    'Easiest, fastest and cheapest way to ' +
    'buy crypto with card and bank transfers.',
  braveWalletBuyTransakDescription:
    'Instant buy with your bank account. Lower fees.',
  braveWalletBuyStripeDescription: 'Pay with credit, debit, bank account',
  braveWalletBuyCoinbaseDescription:
    'Buy with the most trusted name in crypto.',
  braveWalletBuyWithRamp: 'Buy with Ramp',
  braveWalletBuyWithSardine: 'Buy with Sardine',
  braveWalletBuyWithTransak: 'Buy with Transak',
  braveWalletBuyWithStripe: 'Buy with Link',
  braveWalletBuyWithCoinbase: 'Buy with Coinbase Pay',
  braveWalletSellWithProvider: 'Sell with $1',
  braveWalletBuyDisclaimer:
    'Financial and transaction data is processed by our onramp partners. ' +
    'Brave does not collect or have access to such data.',

  // Fund Wallet Screen
  braveWalletFundWalletTitle:
    'To finish your $1 purchase, select one of our partners',
  braveWalletFundWalletDescription:
    'On completion, your funds will be transfered to your Brave Wallet',

  // Deposit Funds Screen
  braveWalletDepositFundsTitle: 'Deposit crypto',
  braveWalletDepositX: 'Deposit $1',
  braveWalletDepositSolSplTokens: 'Deposit Solana or SPL tokens',
  braveWalletDepositErc: 'Deposit ERC-based tokens',
  braveWalletDepositOnlySendOnXNetwork:
    'Only send tokens to this address on $1',

  // Sign Transaction Panel
  braveWalletSignTransactionTitle: 'Your signature is being requested',
  braveWalletSignWarning:
    'Note that Brave can’t verify what will happen if you sign. ' +
    'A signature could authorize nearly any operation in your account or ' +
    'on your behalf, including (but not limited to) giving total control ' +
    'of your account and crypto assets to the site making the request. ' +
    'Only sign if you’re sure you want to take this action, ' +
    'and trust the requesting site.',
  braveWalletSignWarningTitle: 'Sign at your own risk',
  braveWalletSignTransactionMessageTitle: 'Message',
  braveWalletSignTransactionEIP712MessageHideDetails: 'Hide details',
  braveWalletSignTransactionEIP712MessageDomain: 'Domain',
  braveWalletSignTransactionButton: 'Sign',
  braveWalletApproveTransaction: 'Approve transaction',
  braveWalletRiskOfLossAction: 'Unsafe action with risk of loss',
  braveWalletUntrustedRequestWarning:
    'This request appears malicious, ' +
    'and may leave you vulnerable to theft. ' +
    'Do not continue unless you trust the app.',

  // Sign in with Ethereum
  braveWalletSignInWithBraveWallet: 'Sign in with Brave Wallet',
  braveWalletSignInWithBraveWalletMessage:
    'You are signing into $1. ' +
    'Brave Wallet will share your wallet address with $1.',
  braveWalletSeeDetails: 'See details',
  braveWalletSignIn: 'Sign in',
  braveWalletOrigin: 'Origin',
  braveWalletAddress: 'Address',
  braveWalletStatement: 'Statement',
  braveWalletUri: 'URI',
  braveWalletVersion: 'Version',
  braveWalletNonce: 'Nonce',
  braveWalletIssuedAt: 'Issued at',
  braveWalletExpirationTime: 'Expiration time',
  braveWalletNotBefore: 'Not before',
  braveWalletRequestId: 'Request ID',
  braveWalletResources: 'Resources',
  braveWalletSecurityRiskDetected: 'Security risk detected',

  // Encryption Key Panel
  braveWalletProvideEncryptionKeyTitle:
    'A DApp is requesting your public encryption key',
  braveWalletProvideEncryptionKeyDescription:
    '$1$url$2 is requesting your wallets public encryption key. ' +
    'If you consent to providing this key, the site will be able to ' +
    'compose encrypted messages to you.',
  braveWalletProvideEncryptionKeyButton: 'Provide',
  braveWalletReadEncryptedMessageTitle:
    'This DApp would like to read this message to complete your request',
  braveWalletReadEncryptedMessageDecryptButton: 'Decrypt message',
  braveWalletReadEncryptedMessageButton: 'Allow',

  // Allow Spend ERC20 Panel
  braveWalletAllowSpendTitle: 'Allow this app to spend your $1?',
  braveWalletAllowSpendDescription:
    'By granting this permission, you are allowing this app to withdraw ' +
    'your $1 and automate transactions for you.',
  braveWalletAllowSpendBoxTitle: 'Edit permissions',
  braveWalletAllowSpendTransactionFee: 'Transaction fee',
  braveWalletAllowSpendEditButton: 'Edit',
  braveWalletAllowSpendDetailsButton: 'View details',
  braveWalletAllowSpendRejectButton: 'Reject',
  braveWalletAllowSpendConfirmButton: 'Confirm',
  braveWalletAllowSpendUnlimitedWarningTitle: 'Unlimited approval requested',

  // Allow Add or Change Network Panel
  braveWalletAllowAddNetworkTitle: 'Allow this site to add a network?',
  braveWalletAllowAddNetworkDescription:
    'This will allow this network to be used within Brave Wallet.',
  braveWalletAllowAddNetworkLearnMoreButton: 'Learn more.',
  braveWalletAllowAddNetworkName: 'Network name',
  braveWalletAllowAddNetworkUrl: 'Network URL',
  braveWalletAllowAddNetworkDetailsButton: 'View all details',
  braveWalletAllowAddNetworkButton: 'Approve',
  braveWalletChainId: 'Chain ID',
  braveWalletAllowAddNetworkCurrencySymbol: 'Currency symbol',
  braveWalletAllowAddNetworkExplorer: 'Block explorer URL',
  braveWalletAllowChangeNetworkTitle: 'Allow this site to switch the network?',
  braveWalletAllowChangeNetworkDescription:
    'This will switch the network to a previously added network.',
  braveWalletAllowChangeNetworkButton: 'Switch network',
  braveWalletAllowAddNetworkNetworkPanelTitle: 'Network',

  // Confirm Transaction Panel
  braveWalletConfirmTransactionTotal: 'Total',
  braveWalletConfirmTransactionGasFee: 'Gas fee',
  braveWalletConfirmTransactionTransactionFee: 'Transaction fee',
  braveWalletConfirmTransactionBid: 'Bid',
  braveWalletConfirmTransactionAmountGas: 'Amount + gas',
  braveWalletConfirmTransactionAmountFee: 'Amount + fee',
  braveWalletConfirmTransactionNoData: 'No data.',
  braveWalletConfirmTransactionNext: 'next',
  braveWalletConfirmTransactionFirst: 'first',
  braveWalletConfirmTransactions: 'transactions',
  braveWalletConfirmTransactionAccountCreationFee:
    'The associated token account does not exist yet. ' +
    'A small amount of SOL will be spent to create and fund it.',
  braveWalletAllowSpendCurrentAllowance: 'Current allowance',
  braveWalletAllowSpendProposedAllowance: 'Proposed allowance',
  braveWalletTransactionGasLimit: 'Gas Limit',
  braveWalletTransactionGasPremium: 'Gas Premium',
  braveWalletTransactionGasFeeCap: 'Gas Fee Cap',
  braveWalletNetworkFees: 'Network fees',

  // Wallet Main Panel
  braveWalletPanelTitle: 'Brave Wallet',
  braveWalletPanelConnected: 'Connected',
  braveWalletPanelBlocked: 'Blocked',
  braveWalletTitle: 'Wallet',

  // Wallet Welcome Panel
  braveWalletWelcomePanelDescription:
    'Use this panel to securely access Web3 and all your crypto assets.',

  // Site Permissions Panel
  braveWalletSitePermissionsAccounts: '$1 accounts connected',
  braveWalletSitePermissionsDisconnect: 'Disconnect',
  braveWalletSitePermissionsSwitch: 'Switch',
  braveWalletSitePermissionsNewAccount: 'New account',
  braveWalletSitePermissionsTrust: 'Trust',
  braveWalletSitePermissionsRevoke: 'Revoke',

  // DApp Connection Panel
  braveWalletChangeNetwork: 'Change network',
  braveWalletChangeAccount: 'Change account',
  braveWalletActive: 'Active',
  braveWalletNotConnected: 'Not connected',
  braveWalletConnectedAccounts: 'Connected accounts',
  braveWalletAvailableAccounts: 'Available accounts',

  // Transaction Detail Box
  braveWalletTransactionDetailBoxFunction: 'FUNCTION TYPE',
  braveWalletTransactionDetailBoxHex: 'HEX DATA',
  braveWalletTransactionDetailBoxBytes: 'BYTES',

  // Connect With Site Panel
  braveWalletConnectWithSiteNext: 'Next',
  braveWalletConnectWallet: 'Connect wallet',
  braveWalletConnectWithSite: 'or connect with:',
  braveWalletConnectPermittedLabel: 'This app will be able to:',
  braveWalletConnectNotPermittedLabel: 'It will not be able to:',
  braveWalletConnectPermissionBalanceAndActivity:
    'Check wallet balance and activity',
  braveWalletConnectPermissionRequestApproval:
    'Request approval for transactions and signatures',
  braveWalletConnectPermissionAddress: 'View your permitted wallet address',
  braveWalletConnectPermissionMoveFunds: 'Move funds without your permission',
  braveWalletConnectTrustWarning: 'Make sure you trust this site.',

  // Permission Duration
  braveWalletPermissionDuration: 'Permission duration',
  braveWalletPermissionUntilClose: 'Until I close this site',
  braveWalletPermissionOneDay: 'For 24 hours',
  braveWalletPermissionOneWeek: 'For 1 week',
  braveWalletPermissionForever: 'Forever',

  // Import from Legacy Wallet
  braveWalletCryptoWalletsDetected: 'Existing crypto wallets detected',
  braveWalletCryptoWalletsDescriptionTwo:
    'If youd rather skip the import and keep the old Crypto Wallets ' +
    'experience, just navigate to the Brave Browser $1Settings$2 and ' +
    'change the default back to Crypto Wallets. You can also import, ' +
    'try the new Brave Wallet, and change back at any time.',
  braveWalletImportBraveLegacyDescription:
    'Enter your existing crypto wallets password to import to ' +
    'Brave Wallet. Enjoy a faster and more secure way to manage crypto ' +
    'assets and interact with Web3 DApps.',
  braveWalletImportBraveLegacyInput: 'Type Crypto wallets password',

  // Connect Hardware Wallet Panel
  braveWalletConnectHardwarePanelConnected: '$1 connected',
  braveWalletConnectHardwarePanelDisconnected: '$1 disconnected',
  braveWalletConnectHardwarePanelInstructions: 'Instructions',
  braveWalletConnectHardwarePanelConnect: 'Connect your $1',
  braveWalletConnectHardwarePanelConfirmation:
    'Hardware wallet requires transaction confirmation on device.',
  braveWalletConnectHardwarePanelOpenApp:
    'Hardware wallet requires $1 App opened on $2',

  // Transaction History Panel (Empty)
  braveWalletNoTransactionsYet: 'No transaction history',
  braveWalletNoTransactionsYetDescription:
    'Transactions made with your Brave Wallet will appear here.',

  // Transaction List Item
  braveWalletTransactionSent: 'sent',
  braveWalletTransactionReceived: 'received',
  braveWalletTransactionExplorerMissing:
    'Block explorer URL' + ' is not available.',
  braveWalletTransactionExplorer: 'View on block explorer',
  braveWalletTransactionCopyHash: 'Copy transaction hash',
  braveWalletTransactionSpeedup: 'Speedup',
  braveWalletTransactionCancel: 'Cancel transaction',
  braveWalletTransactionRetry: 'Retry transaction',
  braveWalletTransactionPlaceholder: 'Transactions will appear here',
  braveWalletTransactionApproveUnlimited: 'Unlimited',
  braveWalletApprovalTransactionIntent: 'approve',

  // Transaction Simulation Events
  braveWalletReceive: 'Receive',
  braveWalletFrom: 'From',
  braveWalletUnlimitedAssetAmount: 'Unlimited $1',
  braveWalletTokenIsUnverified: 'This token is unverified',
  braveWalletTokenIsVerified: 'This token is verified',
  braveWalletTokenIsVerifiedByLists: 'This token is verified on $1 lists',
  braveWalletSpenderAddress: 'Spender: $1',

  // Asset Detail Accounts (Empty)
  braveWalletNoAccountsWithABalance: 'No available accounts',
  braveWalletNoAccountsWithABalanceDescription:
    'Accounts with a balance will appear here.',

  // Edit Gas
  braveWalletEditGasTitle1: 'Max priority fee',
  braveWalletEditGasTitle2: 'Edit gas',
  braveWalletEditGasDescription:
    'While not a guarantee, miners will likely prioritize your ' +
    'transaction if you pay a higher fee.',
  braveWalletEditGasLow: 'Low',
  braveWalletEditGasOptimal: 'Optimal',
  braveWalletEditGasHigh: 'High',
  braveWalletEditGasLimit: 'Gas limit',
  braveWalletEditGasPerGasPrice: 'Per-gas price (Gwei)',
  braveWalletEditGasPerTipLimit: 'Per-gas tip limit (Gwei)',
  braveWalletEditGasPerPriceLimit: 'Per-gas price limit (Gwei)',
  braveWalletEditGasMaxFee: 'Max fee',
  braveWalletEditGasMaximumFee: 'Maximum fee',
  braveWalletEditGasBaseFee: 'Current base fee',
  braveWalletEditGasGwei: 'Gwei',
  braveWalletEditGasSetCustom: 'Set custom',
  braveWalletEditGasSetSuggested: 'Set suggested',
  braveWalletEditGasZeroGasPriceWarning:
    'Transaction may not be propagated in the network.',
  braveWalletEditGasLimitError: 'Gas limit must be an integer greater than 0',
  braveWalletGasFeeLimitLowerThanBaseFeeWarning:
    'Fee limit is set lower than the base fee. ' +
    'Your transaction may take a long time or fail.',

  // Advanced transaction settings
  braveWalletAdvancedTransactionSettings: 'Advanced settings',
  braveWalletAdvancedTransactionSettingsPlaceholder:
    'Enter custom ' + 'nonce value',
  braveWalletEditNonce: 'Nonce',
  braveWalletEditNonceInfo:
    'The nonce value will be auto-determined if this field is not specified.',

  // Edit permissions
  braveWalletEditPermissionsTitle: 'Edit permissions',
  braveWalletEditPermissionsDescription:
    'Spend limit permission allows $1 to withdraw and spend up to the ' +
    'following amount:',
  braveWalletEditPermissionsButton: 'Edit permissions',
  braveWalletEditPermissionsProposedAllowance: 'Proposed allowance',
  braveWalletEditPermissionsCustomAllowance: 'Custom allowance',

  // Send Input Errors
  braveWalletNotValidFilAddress: 'Not a valid FIL address',
  braveWalletNotValidEthAddress: 'Not a valid ETH address',
  braveWalletNotValidSolAddress: 'Not a valid SOL address',
  braveWalletNotValidAddress: 'Not a valid $1 address.',
  braveWalletNotDomain: "Domain doesn't have a linked $1 address.",
  braveWalletSameAddressError: 'The receiving address is your own address.',
  braveWalletContractAddressError:
    'The receiving address is a tokens contract address.',
  braveWalletFailedChecksumTitle: 'Address doesn’t look correct',
  braveWalletFailedChecksumDescription:
    'Check your address to make sure it’s the right address ' +
    '(e.g. letters with lower or upper case).',
  braveWalletHowToSolve: 'How can I solve it?',
  braveWalletAddressMissingChecksumInfoWarning:
    'This address cannot be verified (missing checksum). Proceed?',
  braveWalletNotValidChecksumAddressError:
    'Address did not pass verification (invalid checksum). ' +
    'Please try again, replacing lowercase letters with uppercase.',
  braveWalletMissingGasLimitError: 'Missing gas limit',
  braveWalletZeroBalanceError: 'Amount must be greater than 0',
  braveWalletAddressRequiredError: 'To address is required',
  braveWalletChecksumModalTitle: 'How can I find the right address?',
  braveWalletChecksumModalDescription:
    'Brave prevents users from sending funds to the wrong address due to ' +
    'incorrect capitalization. Make sure that it’s the correct address by ' +
    'following the instructions below.',
  braveWalletChecksumModalStepOneTitle: '1. Visit',
  braveWalletChecksumModalStepOneDescription:
    'Visit etherscan and paste the wallet address you want to send ' +
    'tokens. Then enter.',
  braveWalletChecksumModalStepTwoTitle: '2. Copy and enter ETH address',
  braveWalletChecksumModalStepTwoDescription:
    'Copy and enter the correct address. ' +
    'You can see that some characters have been converted correctly.',
  braveWalletChecksumModalNeedHelp: 'Need more help?',

  // Transaction Queue Strings
  braveWalletQueueOf: 'of',
  braveWalletQueueNext: 'next',
  braveWalletQueueFirst: 'first',
  braveWalletQueueRejectAll: 'Reject transactions',

  // Add Suggested Token Panel
  braveWalletAddSuggestedTokenTitle: 'Add suggested token',
  braveWalletAddSuggestedTokenDescription:
    'Would you like to import this token?',

  // Transaction Detail Panel
  braveWalletRecentTransactions: 'Recent transactions',
  braveWalletTransactionDetailHash: 'Transaction hash',
  braveWalletTransactionDetailNetwork: 'Network',

  // Transactions Status
  braveWalletTransactionStatusUnapproved: 'Unapproved',
  braveWalletTransactionStatusApproved: 'Approved',
  braveWalletTransactionStatusRejected: 'Rejected',
  braveWalletTransactionStatusSubmitted: 'Submitted',
  braveWalletTransactionStatusConfirmed: 'Confirmed',
  braveWalletTransactionStatusError: 'Error',
  braveWalletTransactionStatusDropped: 'Dropped',
  braveWalletTransactionStatusSigned: 'Signed',

  // Transaction Details
  braveWalletOn: 'On',

  // NFT Details Page
  braveWalletNFTDetailBlockchain: 'Blockchain',
  braveWalletNFTDetailTokenStandard: 'Token standard',
  braveWalletNFTDetailTokenID: 'Token ID',
  braveWalletNFTDetailContractAddress: 'Contract address',
  braveWalletNFTDetailDescription: 'Description',
  braveWalletNFTDetailImageAddress: 'Image URL',
  braveWalletNFTDetailsNotAvailable: 'Not available yet',
  braveWalletNFTDetailsOverview: 'Overview',
  braveWalletNFTDetailsOwnedBy: 'Owned by',
  braveWalletNFTDetailsViewAccount: 'View account',
  braveWalletNFTDetailsPinningStatusLabel: 'IPFS pinning status',
  braveWalletNFTDetailsCid: 'CID',
  braveWalletNFTDetailsProperties: 'Properties',

  // Sweepstakes
  braveWalletSweepstakesTitle: 'Brave Swap-stakes',
  braveWalletSweepstakesDescription:
    '7 days of crypto giveaways, ~$500k in prizes.',
  braveWalletSweepstakesCallToAction: 'Enter now!',

  // Market Data Filters
  braveWalletMarketDataAllAssetsFilter: 'All Assets',
  braveWalletMarketDataTradableFilter: 'Tradable',
  braveWalletMarketDataAssetsColumn: 'Assets',
  braveWalletMarketDataPriceColumn: 'Price',
  braveWalletMarketData24HrColumn: '24Hr',
  braveWalletMarketDataMarketCapColumn: 'Cap',
  braveWalletMarketDataVolumeColumn: 'Volume',
  braveWalletMarketDataBuyDepositColumn: 'Buy/Deposit',

  // Market Coin Detail Screen
  braveWalletInformation: 'Information',
  braveWalletRankStat: 'Rank',
  braveWalletVolumeStat: '24h Volume',
  braveWalletMarketCapStat: 'Market Cap',

  // Network Filter
  braveWalletNetworkFilterAll: 'All Networks',
  braveWalletNetworkFilterSecondary: 'Secondary networks',
  braveWalletNetworkFilterTestNetworks: 'Test networks',

  // Asset Filter
  braveWalletAssetFilterLowToHigh: 'Low to High',
  braveWalletAssetFilterHighToLow: 'High to Low',
  braveWalletAssetFilterAToZ: 'A to Z',
  braveWalletAssetFilterZToA: 'Z to A',

  // Asset Group By
  braveWalletNone: 'None',
  braveWalletNetworks: 'Networks',
  braveWalletPortfolioGroupByTitle: 'Group by',
  braveWalletGroupByCollection: 'Group by collection',
  braveWalletPortfolioGroupByDescription: 'Group assets by',

  // Portfolio Filters
  braveWalletPortfolioFiltersTitle: 'Filters and display settings',
  braveWalletPortfolioNftsFiltersTitle: 'NFT display settings',
  braveWalletSortAssets: 'Sort assets',
  braveWalletSortAssetsDescription: 'Sort by fiat value or name',
  braveWalletHideSmallBalances: 'Hide small balances',
  braveWalletHideSmallBalancesDescription: 'Assets with value less than $1',
  braveWalletSelectAccounts: 'Select accounts',
  braveWalletSelectNetworks: 'Select networks',
  braveWalletSelectAll: 'Select all',
  braveWalletDeselectAll: 'Deselect all',
  braveWalletPrimaryNetworks: 'Primary networks',
  braveWalletETHAccountDescription: 'Ethereum + EVM Chains',
  braveWalletSOLAccountDescription: 'Solana + SVM Chains',
  braveWalletFILAccountDescription: 'Filecoin',
  braveWalletBTCMainnetAccountDescription: 'Bitcoin Mainnet',
  braveWalletBTCTestnetAccountDescription: 'Bitcoin Testnet',
  braveWalletZECAccountDescription: 'Zcash Mainnet',
  braveWalletZECTestnetAccountDescription: 'Zcash Testnet',
  braveWalletShowNetworkLogoOnNftsTitle: 'Network Logo',
  braveWalletShowNetworkLogoOnNftsDescription: 'Show network logo on NFTs',
  braveWalletShowSpamNftsTitle: 'Spam NFTs',
  braveWalletShowSpamNftsDescription: 'Show Spam NFTs',
  braveWalletPortfolioSettings: 'Portfolio Settings',
  braveWalletHideNotOwnedNfTs: 'Hide not owned NFTs',

  // Account Filter
  braveWalletAccountFilterAllAccounts: 'All accounts',

  // Transaction post-confirmation

  // Submitted
  braveWalletTransactionSubmittedTitle: 'Transaction submitted',
  braveWalletTransactionSubmittedDescription:
    'Transaction has been successfully sent ' +
    'to the network and awaits confirmation.',

  // Failed
  braveWalletTransactionFailedHeaderTitle: '$1 was returned to your wallet',
  braveWalletTransactionFailedTitle: 'Transaction failed',
  braveWalletTransactionFailedDescription:
    'Transaction was failed due to a large price movement. ' +
    'Increase slippage tolerance to succeed at a larger price movement.',
  braveWalletTransactionFailedSwapNextCTA: 'New trade',
  braveWalletTransactionFailedNextCTA: 'New transaction',
  braveWalletTransactionFailedViewErrorCTA: 'View error',
  braveWalletTransactionFailedReceiptCTA: 'Receipt',
  braveWalletTransactionFailedModalTitle: 'Error message',
  braveWalletTransactionFailedModalSubtitle:
    'Please save the error message for future reference.',

  // Complete
  braveWalletTransactionCompleteSwapHeaderTitle: 'Swapped $1 to $2',
  braveWalletTransactionCompleteTitle: 'Transaction complete!',
  braveWalletTransactionCompleteDescription:
    'Transaction was successful. Please wait for confirmations, ' +
    'to avoid the risk of double-spend.',
  braveWalletTransactionCompleteReceiptCTA: 'Receipt',

  // Confirming
  braveWalletTransactionConfirmingTitle: 'Transaction is processing',
  // [FIXME]: change the wording after ETH2.
  braveWalletTransactionConfirmingDescription:
    'Transaction was successfully included in a block. ' +
    'To avoid the risk of double spending, ' +
    'we recommend waiting for block confirmations.',
  braveWalletTransactionConfirmingText: 'Confirming',

  // Transaction intents for confirmation panels
  braveWalletTransactionIntentDappInteraction: 'Dapp interaction',
  braveWalletTransactionIntentSend: 'Send $1',
  braveWalletTransactionIntentSwap: 'Swap $1 to $2',

  // Solana ProgramID Names
  braveWalletSolanaSystemProgram: 'System Program',
  braveWalletSolanaConfigProgram: 'Config Program',
  braveWalletSolanaStakeProgram: 'Stake Program',
  braveWalletSolanaVoteProgram: 'Vote Program',
  braveWalletSolanaBPFLoader: 'BPF Loader',
  braveWalletSolanaEd25519Program: 'Ed25519 Program',
  braveWalletSolanaSecp256k1Program: 'Secp256k1 Program',
  braveWalletSolanaTokenProgram: 'Token Program',
  braveWalletSolanaAssociatedTokenProgram: 'Associated Token Program',
  braveWalletSolanaMetaDataProgram: 'MetaData Program',
  braveWalletSolanaSysvarRentProgram: 'SysvarRent Program',

  // Solana Instruction Parameter Names
  braveWalletSolanaAccounts: 'Accounts:',
  braveWalletSolanaData: 'Data:',
  braveWalletSolanaProgramID: 'Program ID:',
  braveWalletSolanaMaxRetries: 'Max Retries:',
  braveWalletSolanaPreflightCommitment: 'Preflight Commitment:',
  braveWalletSolanaSkipPreflight: 'Skip Preflight:',
  braveWalletSolanaAddressLookupTableAccount: 'Address Lookup Table Account:',
  braveWalletSolanaAddressLookupTableIndex: 'Address Lookup Table Index:',

  // Help Center
  braveWalletHelpCenter: 'Help center',
  braveWalletHelpCenterText: 'Need help? See',

  // Remove Account Modal
  braveWalletRemoveAccountModalTitle: 'Are you sure you want to remove "$1"?',

  // Bridge to Aurora
  braveWalletAuroraModalTitle: 'Open the Rainbow Bridge app?',
  braveWalletAuroraModalDescription:
    'Rainbow Bridge is an independent service that helps you bridge ' +
    'assets across networks, and use your crypto on other networks ' +
    'and DApp ecosystems. Bridging assets to other networks has some risks.',
  braveWalletAuroraModalLearnMore: 'Learn more about using Rainbow Bridge',
  braveWalletAuroraModalLearnMoreAboutRisk:
    'Learn more about mitigating risk on Rainbow Bridge',
  braveWalletAuroraModalDontShowAgain: "Don't show again",
  braveWalletAuroraModalOPenButtonText: 'Open the Rainbow Bridge app',
  braveWalletBridgeToAuroraButton: 'Bridge to Aurora',

  // Input field labels
  braveWalletInputLabelPassword: 'Password',

  // Wallet Welcome Perks
  braveWalletPerksTokens: 'Buy, send, and swap 200+ crypto assets',
  braveWalletMultiChain: 'Multi-chain & NFT support',
  braveWalletPerksBrowserNative: 'Browser-native, no risky extensions',

  // Portfolio Asset More Popup
  braveWalletPortfolioTokenDetailsMenuLabel: 'Token details',
  braveWalletPortfolioViewOnExplorerMenuLabel: 'View on block explorer',
  braveWalletPortfolioHideTokenMenuLabel: 'Hide token',
  braveWalletHideTokenModalTitle: 'Hide token',

  // Token detail modals
  braveWalletMakeTokenVisibleInstructions:
    'You can make this asset visible again in the future by clicking ' +
    'the "+ Visible assets" button at the bottom of the "Portfolio" tab',
  braveWalletConfirmHidingToken: 'Hide',
  braveWalletCancelHidingToken: 'Cancel',

  // Visible assets modal
  braveWalletMyAssets: 'My assets',
  braveWalletAvailableAssets: 'Available assets',
  braveWalletDidntFindAssetEndOfList: "Didn't find your asset on the list?",
  braveWalletDidntFindAssetInList:
    "If you didn't find your asset in this list, you can add it manually " +
    'by using the button below',
  braveWalletAssetNotFound: 'Asset not found',

  // Request feature button
  braveWalletRequestFeatureButtonText: 'Request feature',

  // Warnings
  braveWalletNonAsciiCharactersInMessageWarning:
    'Non-ASCII characters detected!',
  braveWalletFoundRisks: 'We found $1 risks.',
  braveWalletFoundIssues: 'We found $1 issues.',

  // ASCII toggles
  braveWalletViewEncodedMessage: 'View original message',
  braveWalletViewDecodedMessage: 'View message in ASCII encoding',

  // NFTs Tab
  braveNftsTabImportNft: 'Import NFT',
  braveNftsTabEmptyStateHeading: 'No NFTs here yet.',
  braveNftsTabEmptyStateSubHeading:
    'Ready to add some? Just click the button below to import.',
  braveNftsTabEmptyStateDisclaimer:
    'Compatible with NFTs on Solana (SPL) and Ethereum (ERC-721).',
  braveNftsTab: 'NFTs',
  braveNftsTabHidden: 'Hidden',
  braveNftsTabCollected: 'Collected',
  braveNftsTabHide: 'Hide',
  braveNftsTabUnhide: 'Unhide',
  braveNftsTabEdit: 'Edit',
  braveNftsTabRemove: "Don't show in wallet",

  // Add asset tabs
  braveWalletAddAssetTokenTabTitle: 'Token',
  braveWalletAddAssetNftTabTitle: 'NFT',
  braveWalletNftFetchingError:
    'Something went wrong when fetching NFT details. Please try again later.',
  braveWalletEditToken: 'Edit Token',

  // Add Custom Asset Form
  braveWalletNetworkIsRequiredError: 'Network is required',
  braveWalletTokenNameIsRequiredError: 'Token name is required',
  braveWalletInvalidTokenContractAddressError:
    'Invalid or empty token contract address',
  braveWalletTokenSymbolIsRequiredError: 'Token symbol is required',
  braveWalletTokenDecimalsIsRequiredError:
    'Token decimals of precision value is required',
  braveWalletTokenContractAddress: 'Token Contract Address',
  braveWalletTokenDecimal: 'Token Decimal',
  braveWalletTokenMintAddress: 'Mint address',
  braveWalletTransactionHasFeeEstimatesError: 'Unable to fetch fee estimates',

  braveWalletEditNftModalTitle: 'Edit NFT',
  braveWalletNftMoveToSpam: 'Mark as junk',
  braveWalletNftUnspam: 'Mark as not junk',

  // NFT Labels
  braveWalletNftJunk: 'Junk',
  braveWalletWatchOnly: 'Watch-only',

  // Add NFT modal
  braveWalletAddNftModalTitle: 'Add NFT',
  braveWalletAddNftModalDescription:
    "If you can't see an NFT automatically detected by Brave, " +
    'or want to track one owned by others, you can import it manually.',
  braveWalletWhatIsAnNftContractAddress:
    'The contract address when adding an NFT to a wallet is the unique ' +
    'address on the blockchain where the smart contract ' +
    'for the NFT collection resides. ' +
    'You can find the contract address from marketplace, or ' +
    'blockchain explorers, etc.',
  braveWalletWhatIsAnNftTokenId:
    'The unique identifier for the specific NFT within the collection.',
  braveWalletNftNameFieldExplanation:
    'The token name for an NFT refers to the specific name given to a ' +
    'non-fungible token within a particular collection or project. ' +
    'This name distinguishes one NFT from another within the same collection.',
  braveWalletNftSymbolFieldExplanation:
    'The NFT symbol refers to the unique identifier ' +
    'used to distinguish the NFT collection or project.',
  braveWalletFetchNftMetadataError: 'Unable to fetch NFT metadata',
  braveWalletWatchThisNft: 'Watch this NFT',
  braveWalletUnownedNftAlert:
    'You do not own this NFT. ' +
    'While you can still add it to your wallet, ' +
    'you will not be able to make any transactions with it.',

  // Remove NFT modal
  braveWalletRemoveNftModalHeader: 'Remove from Brave Wallet?',
  braveWalletRemoveNftModalDescription:
    'NFT will be removed from Brave Wallet but will remain on the ' +
    'blockchain. If you remove it, then change your mind, ' +
    "you'll need to import it again manually.",
  braveWalletRemoveNftModalCancel: 'Cancel',
  braveWalletRemoveNftModalConfirm: 'Remove',

  // NFT auto discovery modal
  braveWalletEnableNftAutoDiscoveryModalHeader:
    'Want your NFTs displayed automatically?',
  braveWalletEnableNftAutoDiscoveryModalDescription:
    'Brave Wallet can use a third-party service to automatically display ' +
    'your NFTs. Brave will share your wallet addresses with ' +
    '$1SimpleHash$2 to provide this service. $3Learn more.$4',
  braveWalletEnableNftAutoDiscoveryModalConfirm: 'Yes, proceed',
  braveWalletEnableNftAutoDiscoveryModalCancel:
    "No thanks, I'll do it manually",
  braveWalletAutoDiscoveryEmptyStateHeading: 'No NFTs to display',
  braveWalletAutoDiscoveryEmptyStateSubHeading:
    'Once an NFT is detected, it’ll be displayed here.',
  braveWalletAutoDiscoveryEmptyStateFooter: 'Can’t see your NFTs?',
  braveWalletAutoDiscoveryEmptyStateActions:
    '$1Refresh$2 or $3Import Manually$4',
  braveWalletAutoDiscoveryEmptyStateRefresh: 'Refreshing',

  // Brave Wallet Rewards
  braveWalletUphold: 'Uphold',
  braveWalletGemini: 'Gemini',
  braveWalletZebpay: 'Zebpay',
  braveWalletBitflyer: 'bitFlyer',
  braveWalletRewardsAccount: '$1 account',
  braveWalletBraveRewardsTitle: 'Brave Rewards',
  braveWalletBraveRewardsDescription: 'Brave Rewards BAT on $1',
  braveWalletBraveRewardsLoggedOutDescription:
    'You’re currently logged out of $1. Please log in to view your balance.',
  braveWalletLogIn: 'Log in',
  braveWalletViewOn: 'View on $1',
  braveWalletRewardsSettings: 'Rewards settings',
  braveWalletPlatforms: 'Platforms',
  braveWalletTestNetworkAccount: 'Test Network Account',

  // Transaction Simulations Opt-in
  braveWalletEnableEnhancedTransactionDetailsTitle:
    'Enable Enhanced Transaction Details to:',
  braveWalletTransactionSimulationSeeEstimates: 'See estimated balance change',
  braveWalletTransactionSimulationDetectMalicious:
    'Identify potentially malicious transactions',
  braveWalletTransactionSimulationDetectPhishing:
    'Help detect phishing attempts',
  braveWalletTransactionSimulationTerms:
    'Brave works with a partner to provide this service. ' +
    'By enabling, you agree to share unsigned transactions ' +
    'and their DApp URLs with our partner.',

  // Settings
  braveWalletChangeAnytimeInSettings: 'Change anytime in $1Settings$2.',

  // Balance Details Modal
  braveWalletAvailable: 'Available',
  braveWalletAvailableBalanceDescription: 'Funds available for you to use.',
  braveWalletPending: 'Pending',
  braveWalletPendingBalanceDescription:
    'A pending change in your wallet balance.',
  braveWalletTotalBalanceDescription:
    'Your available funds plus any not-yet-confirmed transactions.',
  braveWalletUnavailableBalances: 'Some balances may be unavailable',

  // Misc. placeholders
  braveWalletExempliGratia: 'e.g $1',

  // Explore
  braveWalletWeb3: 'Web3',
  braveWalletTopNavExplore: 'Explore',
  braveWalletNoDappsFound: 'No DApps found',
  braveWalletActiveWallets: 'Active wallets',
  braveWalletVisitDapp: 'Visit $1',
  braveWalletFilters: 'Filters',
  braveWalletClearFilters: 'Clear filters',
  braveWalletShowMore: 'Show more',
  braveWalletDetails: 'Details'
})
