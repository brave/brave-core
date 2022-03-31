import { provideStrings } from '../../../.storybook/locale'

provideStrings({
  // App Categories
  braveWalletDefiCategory: 'Defi apps',
  braveWalletNftCategory: 'NFT marketplaces',
  braveWalletSearchCategory: 'Search results',

  // App Category Button Text
  braveWalletDefiButtonText: 'Browse more Defi',
  braveWalletNftButtonText: 'Browse more NFT',

  // Compound App
  braveWalletCompoundName: 'Compound',
  braveWalletCompoundDescription: 'Unlock a universe of open financial applications.',

  // Maker App
  braveWalletMakerName: 'MakerDAO',
  braveWalletMakerDescription: 'Maker - stablecoin, loans and governance...',

  // Aave App
  braveWalletAaveName: 'Aave',
  braveWalletAaveDescription: 'Protocol to earn on deposits & borrow assets.',

  // OpenSea App
  braveWalletOpenSeaName: 'OpenSea',
  braveWalletOpenSeaDescription: 'The largest NFT marketplace. Buy, sell, and discover rare digital items',

  // Rarible App
  braveWalletRaribleName: 'Rarible',
  braveWalletRaribleDescription: 'Create and sell digital artworks',

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
  braveWalletChartLive: '1H',
  braveWalletChartOneDay: '1D',
  braveWalletChartOneWeek: '1W',
  braveWalletChartOneMonth: '1M',
  braveWalletChartThreeMonths: '3M',
  braveWalletChartOneYear: '1Y',
  braveWalletChartAllTime: 'All',

  // Portfolio View
  braveWalletAddCoin: 'Add Coin',
  braveWalletBalance: 'Balance',
  braveWalletPortfolioAssetNetworkDescription: '$1 on $2',

  // Portfolio SubView
  braveWalletAccounts: 'Accounts',
  braveWalletAccount: 'Account',
  braveWalletOwner: 'Owner',
  braveWalletTransactions: 'Transactions',
  braveWalletPrice: 'Price',
  braveWalletBack: 'Back',
  braveWalletAddAccount: 'Add Account',
  braveWalletPoweredByCoinGecko: 'Price data powered by CoinGecko',

  // BuySendSwap
  braveWalletBuy: 'Buy',
  braveWalletSend: 'Send',
  braveWalletSwap: 'Swap',
  braveWalletReset: 'Reset',
  braveWalletBuyNotSupportedTooltip: 'Buy not supported',
  braveWalletSwapNotSupportedTooltip: 'Swap not supported',
  braveWalletSlippageToleranceWarning: 'Transaction may be frontrun',
  braveWalletSlippageToleranceTitle: 'Slippage tolerance',
  braveWalletExpiresInTitle: 'Expires in',
  braveWalletSendPlaceholder: '0x address or url',
  braveWalletSwapDisclaimer: 'Brave uses $10x$2 as a DEX aggregator.',
  braveWalletSwapDisclaimerDescription: '0x will process the Ethereum address and IP address to fulfill a transaction (including getting quotes). 0x will ONLY use this data for the purposes of processing transactions.',
  braveWalletSwapFeesNotice: 'Quote includes a 0.875% Brave fee.',
  braveWalletDecimalPlacesError: 'Too many decimal places',

  // Buttons
  braveWalletButtonContinue: 'Continue',
  braveWalletButtonCopy: 'Copy',
  braveWalletButtonCopied: 'Copied',
  braveWalletButtonVerify: 'Verify',

  // Wallet Onboarding Welcome
  braveWalletWelcomeTitle: 'Welcome to the new Brave Wallet',
  braveWalletWelcomeDescription: 'Hold crypto assets in your custody. Track portfolio performance, and interact with web 3 DApps. Trade, invest, borrow, and lend with DeFi. All right from the Brave privacy browser. No extensions, no download required.',
  braveWalletWelcomeButton: 'Get Started',
  braveWalletWelcomeRestoreButton: 'Restore',

  // Backup Wallet Intro
  braveWalletBackupIntroTitle: 'Back up your crypto wallet',
  braveWalletBackupIntroDescription: 'In the next step you’ll see a $1-word recovery phrase, which you can use to recover your primary crypto accounts. Save it someplace safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.',
  braveWalletBackupIntroTerms: 'I understand that if I lose my recovery words, I will not be able to access my crypto wallet.',
  braveWalletBackupButtonSkip: 'Skip',
  braveWalletBackupButtonCancel: 'Cancel',

  // Recovery Phrase Backup
  braveWalletRecoveryTitle: 'Your recovery phrase',
  braveWalletRecoveryDescription: 'Write down or copy these words in the exact order shown below, and save them somewhere safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.',
  braveWalletRecoveryWarning1: 'WARNING:',
  braveWalletRecoveryWarning2: 'Never share your recovery phrase.',
  braveWalletRecoveryWarning3: 'Anyone with this phrase can take your assets forever.',
  braveWalletRecoveryTerms: 'I have backed up my phrase somewhere safe.',

  // Verify Recovery Phrase
  braveWalletVerifyRecoveryTitle: 'Verify recovery phrase',
  braveWalletVerifyRecoveryDescription: 'Select the words in your recovery phrase in their correct order.',
  braveWalletVerifyError: 'Recovery phrase did not match, please try again.',

  // Create Password
  braveWalletCreatePasswordTitle: 'Secure your crypto assets with a password',
  braveWalletCreatePasswordDescription: 'Passwords must be at least 7 characters, and contain at least one number and one special character.',
  braveWalletCreatePasswordInput: 'Password',
  braveWalletConfirmPasswordInput: 'Confirm password',
  braveWalletCreatePasswordError: 'Password criteria doesn\'t match.',
  braveWalletConfirmPasswordError: 'Passwords do not match',

  // Lock Screen
  braveWalletLockScreenTitle: 'Enter password to unlock wallet',
  braveWalletLockScreenButton: 'Unlock',
  braveWalletLockScreenError: 'Incorrect password',

  // Wallet More Popup
  braveWalletWalletPopupSettings: 'Settings',
  braveWalletWalletPopupLock: 'Lock Wallet',
  braveWalletWalletPopupBackup: 'Backup Wallet',
  braveWalletWalletPopupConnectedSites: 'Connected sites',

  // Backup Warning
  braveWalletBackupWarningText: 'Back up your wallet now to protect your assets and ensure you never lose access.',
  braveWalletBackupButton: 'Back up now',
  braveWalletDismissButton: 'Dismiss',

  // Default Wallet Banner
  braveWalletDefaultWalletBanner: 'Brave Wallet is not set as your default wallet and will not respond to web3 DApps. Visit settings to change your default wallet.',

  // Restore Screen
  braveWalletRestoreTite: 'Restore primary crypto accounts',
  braveWalletRestoreDescription: 'Enter your recovery phrase to restore your Brave wallet crypto account.',
  braveWalletRestoreError: 'The recovery phrase entered is invalid.',
  braveWalletRestorePlaceholder: 'Paste recovery phrase from clipboard',
  braveWalletRestoreShowPhrase: 'Show recovery phrase',
  braveWalletRestoreLegacyCheckBox: 'Import from legacy Brave crypto wallets?',
  braveWalletRestoreFormText: 'New Password',

  // Tool Tips
  braveWalletToolTipCopyToClipboard: 'Copy to Clipboard',

  // Accounts Tab
  braveWalletAccountsPrimary: 'Primary crypto accounts',
  braveWalletAccountsPrimaryDisclaimer: 'You can create primary accounts in Brave Wallet that may be backed up or restored from your recovery phrase. To learn more about account types visit account help',
  braveWalletAccountsSecondary: 'Imported accounts',
  braveWalletAccountsSecondaryDisclaimer: 'These accounts can be used with web 3 DApps, and can be shown in your portfolio. However, note that secondary accounts cannot be restored via recovery phrase from your primary account backup.',
  braveWalletAccountsAssets: 'Assets',
  braveWalletAccountsEditVisibleAssets: 'Visible assets',

  // Add Account Options
  braveWalletCreateAccount: 'Create $1 account',
  braveWalletAddAccountCreate: 'Create',
  braveWalletAddAccountImport: 'Import',
  braveWalletAddAccountImportHardware: 'Import from hardware wallet',
  braveWalletAddAccountHardware: 'Hardware',
  braveWalletAddAccountConnect: 'Connect',
  braveWalletAddAccountPlaceholder: 'Account name',
  braveWalletCreateAccountButton: 'Create account',
  braveWalletCreateAccountImportAccount: 'Import $1 account',
  braveWalletCreateAccountTitle: 'Select one of the following account types',
  braveWalletCreateAccountEthereumDescription: 'Supports EVM compatible assets on the Ethereum blockchain (ERC-20, ERC-721, ERC-1551, ERC-1155)',
  braveWalletCreateAccountSolanaDescription: 'Supports SPL compatible assets on the Solana blockchain',
  braveWalletCreateAccountFilecoinDescription: 'Store FIL asset',
  braveWalletFilecoinPrivateKeyProtocol: 'Private key $1',

  // Import Account
  braveWalletImportAccountDisclaimer: 'These accounts can be used with web 3 DApps, and can be shown in your portfolio. However, note that secondary accounts cannot be restored via recovery phrase from your primary account backup.',
  braveWalletImportAccountPlaceholder: 'Paste private key from clipboard',
  braveWalletImportAccountKey: 'Private key',
  braveWalletImportAccountFile: 'JSON file',
  braveWalletImportAccountUploadButton: 'Choose file',
  braveWalletImportAccountUploadPlaceholder: 'No file chosen',
  braveWalletImportAccountError: 'Failed to import account, please try again.',

  // Connect Hardware Wallet
  braveWalletConnectHardwareTitle: 'Select your hardware wallet device',
  braveWalletConnectHardwareInfo1: 'Connect your $1 wallet directly to your computer.',
  braveWalletConnectHardwareInfo2: 'Unlock your device and select the $1 app.',
  braveWalletConnectHardwareTrezor: 'Trezor',
  braveWalletConnectHardwareLedger: 'Ledger',
  braveWalletConnectingHardwareWallet: 'Connecting...',
  braveWalletAddCheckedAccountsHardwareWallet: 'Add checked accounts',
  braveWalletLoadMoreAccountsHardwareWallet: 'Load more',
  braveWalletLoadingMoreAccountsHardwareWallet: 'Loading more...',
  braveWalletSearchScannedAccounts: 'Search account',
  braveWalletSwitchHDPathTextHardwareWallet: 'Try switching HD path (above) if you cannot find the account you are looking for.',
  braveWalletLedgerLiveDerivationPath: 'Ledger Live',
  braveWalletLedgerLegacyDerivationPath: 'Legacy (MEW/MyCrypto)',
  braveWalletUnknownInternalError: 'Unknown error, please reconnect your hardware wallet and try again.',
  braveWalletConnectHardwareSearchNothingFound: 'No results found.',

  // Account Settings Modal
  braveWalletAccountSettingsDetails: 'Details',
  braveWalletAccountSettingsWatchlist: 'Visible assets',
  braveWalletAccountSettingsPrivateKey: 'Private key',
  braveWalletAccountSettingsSave: 'Save',
  braveWalletAccountSettingsRemove: 'Remove account',
  braveWalletWatchlistAddCustomAsset: 'Add custom asset',
  braveWalletWatchListTokenName: 'Token name',
  braveWalletWatchListTokenAddress: 'Token contract address',
  braveWalletWatchListTokenSymbol: 'Token symbol',
  braveWalletWatchListTokenDecimals: 'Decimals of percision',
  braveWalletWatchListAdd: 'Add',
  braveWalletWatchListDoneButton: 'Done',
  braveWalletWatchListSuggestion: 'Add $1 as a custom token',
  braveWalletWatchListNoAsset: 'No assets named',
  braveWalletWatchListSearchPlaceholder: 'Search assets or contract address',
  braveWalletWatchListError: 'Failed to add custom token, please try again.',
  braveWalletAccountSettingsDisclaimer: 'WARNING: Never share your recovery phrase. Anyone with this phrase can take your assets forever.',
  braveWalletAccountSettingsShowKey: 'Show key',
  braveWalletAccountSettingsHideKey: 'Hide key',
  braveWalletAccountSettingsUpdateError: 'Failed to update account name, please try again.',
  braveWalletWatchListTokenId: 'Token ID (only for ERC721)',
  braveWalletWatchListTokenIdError: 'Token ID is required',
  braveWalletWatchListAdvanced: 'Advanced',
  braveWalletWatchListCoingeckoId: 'Coingecko ID',
  braveWalletIconURL: 'Icon URL',
  braveWalletAddAsset: 'Add asset',

  // AmountPresets
  braveWalletPreset25: '25%',
  braveWalletPreset50: '50%',
  braveWalletPreset75: '75%',
  braveWalletPreset100: '100%',

  // Networks
  braveWalletNetworkETH: 'Ethereum',
  braveWalletNetworkMain: 'Mainnet',
  braveWalletNetworkTest: 'Test Network',
  braveWalletNetworkRopsten: 'Roptsten',
  braveWalletNetworkKovan: 'Kovan',
  braveWalletNetworkRinkeby: 'Rinkeby',
  braveWalletNetworkGoerli: 'Goerli',
  braveWalletNetworkBinance: 'Binance Smart Chain',
  braveWalletNetworkBinanceAbbr: 'BSC',
  braveWalletNetworkLocalhost: 'Localhost',

  // Select Screens
  braveWalletSelectAccount: 'Select account',
  braveWalletSearchAccount: 'Search accounts',
  braveWalletSelectNetwork: 'Select network',
  braveWalletSelectAsset: 'Select from',
  braveWalletSearchAsset: 'Search coins',

  // Swap
  braveWalletSwapFrom: 'Amount',
  braveWalletSwapTo: 'To',
  braveWalletSwapEstimate: 'estimate',
  braveWalletSwapMarket: 'Market',
  braveWalletSwapLimit: 'Limit',
  braveWalletSwapPriceIn: 'Price in',
  braveWalletSwapInsufficientBalance: 'Insufficient balance',
  braveWalletSwapInsufficientFundsForGas: 'Insufficient funds for gas',
  braveWalletSwapInsufficientLiquidity: 'Insufficient liquidity',
  braveWalletSwapInsufficientAllowance: 'Activate token',
  braveWalletSwapUnknownError: 'Unknown error',

  // Buy
  braveWalletBuyTitle: 'Test faucet',
  braveWalletBuyDescription: 'Get Ether from a faucet for $1',
  braveWalletBuyWyreButton: 'Continue to Wyre',
  braveWalletBuyFaucetButton: 'Get Ether',

  // Sign Transaction Panel
  braveWalletSignTransactionTitle: 'Your signature is being requested',
  braveWalletSignWarning: 'Note that Brave can’t verify what will happen if you sign. A signature could authorize nearly any operation in your account or on your behalf, including (but not limited to) giving total control of your account and crypto assets to the site making the request. Only sign if you’re sure you want to take this action, and trust the requesting site.',
  braveWalletSignWarningTitle: 'Sign at your own risk',
  braveWalletSignTransactionMessageTitle: 'Message',
  braveWalletSignTransactionButton: 'Sign',

  // Encryption Key Panel
  braveWalletProvideEncryptionKeyTitle: 'A DApp is requesting your public encryption key',
  braveWalletProvideEncryptionKeyDescription: '$1 is requesting your wallets public encryption key. If you consent to providing this key, the site will be able to compose encrypted messages to you.',
  braveWalletProvideEncryptionKeyButton: 'Provide',
  braveWalletReadEncryptedMessageTitle: '$1 would like to read this message to complete your request',
  braveWalletReadEncryptedMessageDecryptButton: 'Decrypt message',
  braveWalletReadEncryptedMessageButton: 'Allow',

  // Allow Spend ERC20 Panel
  braveWalletAllowSpendTitle: 'Allow this app to spend your $1?',
  braveWalletAllowSpendDescription: 'By granting this permission, you are allowing this app to withdraw your $1 and automate transactions for you.',
  braveWalletAllowSpendBoxTitle: 'Edit permissions',
  braveWalletAllowSpendTransactionFee: 'Transaction fee',
  braveWalletAllowSpendEditButton: 'Edit',
  braveWalletAllowSpendDetailsButton: 'View details',
  braveWalletAllowSpendRejectButton: 'Reject',
  braveWalletAllowSpendConfirmButton: 'Confirm',
  braveWalletAllowSpendUnlimitedWarningTitle: 'Unlimited approval requested',

  // Allow Add or Change Network Panel
  braveWalletAllowAddNetworkTitle: 'Allow this site to add a network?',
  braveWalletAllowAddNetworkDescription: 'This will allow this network to be used within Brave Wallet.',
  braveWalletAllowAddNetworkLearnMoreButton: 'Learn more.',
  braveWalletAllowAddNetworkName: 'Network name',
  braveWalletAllowAddNetworkUrl: 'Network URL',
  braveWalletAllowAddNetworkDetailsButton: 'View all details',
  braveWalletAllowAddNetworkButton: 'Approve',
  braveWalletAllowAddNetworkChainID: 'Chain ID',
  braveWalletAllowAddNetworkCurrencySymbol: 'Currency symbol',
  braveWalletAllowAddNetworkExplorer: 'Block explorer URL',
  braveWalletAllowChangeNetworkTitle: 'Allow this site to switch the network?',
  braveWalletAllowChangeNetworkDescription: 'This will switch the network to a previously added network.',
  braveWalletAllowChangeNetworkButton: 'Switch network',

  // Confirm Transaction Panel
  braveWalletConfirmTransactionTotal: 'Total',
  braveWalletConfirmTransactionGasFee: 'Gas fee',
  braveWalletConfirmTransactionBid: 'Bid',
  braveWalletConfirmTransactionAmountGas: 'Amount + gas',
  braveWalletConfirmTransactionNoData: 'No data.',
  braveWalletConfirmTransactionNext: 'next',
  braveWalletConfirmTransactionFrist: 'first',
  braveWalletConfirmTransactions: 'transactions',
  braveWalletAllowSpendCurrentAllowance: 'Current allowance',
  braveWalletAllowSpendProposedAllowance: 'Proposed allowance',

  // Wallet Main Panel
  braveWalletPanelTitle: 'Brave Wallet',
  braveWalletPanelConnected: 'Connected',
  braveWalletPanelNotConnected: 'Connect',
  braveWalletPanelViewAccountAssets: 'View account assets',
  braveWalletAssetsPanelTitle: 'Account assets',

  // Wallet Welcome Panel
  braveWalletWelcomePanelDescription: 'Use this panel to securely access web3 and all your crypto assets.',
  braveWalletWelcomePanelButton: 'Learn more',

  // Site Permissions Panel
  braveWalletSitePermissionsTitle: 'Site permissions',
  braveWalletSitePermissionsAccounts: '$1 accounts connected',
  braveWalletSitePermissionsDisconnect: 'Disconnect',
  braveWalletSitePermissionsSwitch: 'Switch',
  braveWalletSitePermissionsNewAccount: 'New account',

  // Transaction Detail Box
  braveWalletTransactionDetailBoxFunction: 'FUNCTION TYPE',
  braveWalletTransactionDetailBoxHex: 'HEX DATA',
  braveWalletTransactionDetailBoxBytes: 'BYTES',

  // Connect With Site Panel
  braveWalletConnectWithSiteTitle: 'Select accounts(s)',
  braveWalletConnectWithSiteDescription: 'View the addressess of your permitted accounts (required)',
  braveWalletConnectWithSiteNext: 'Next',
  braveWalletConnectWithSiteHeaderTitle: 'Connect with Brave Wallet',

  // Import from Hardware
  braveWalletImportFromExternalNewPassword: 'A more secure password is required for Brave Wallet',
  braveWalletImportFromExternalCreatePassword: 'Set a Brave wallet password',
  braveWalletImportFromExternalPasswordCheck: 'Use the same password',

  // Import from MetaMask
  braveWalletImportTitle: 'Import from $1',
  braveWalletImportMetaMaskTitle: 'MetaMask',
  braveWalletImportDescription: 'Import your $1 accounts into Brave Wallet. Enjoy a faster and more secure way to manage crypto assets and interact with web 3 DApps.',
  braveWalletImportMetaMaskInput: 'MetaMask password',

  // Import from Legacy Wallet
  braveWalletImportBraveLegacyTitle: 'crypto wallets',
  braveWalletCryptoWalletsDetected: 'Existing crypto wallets detected',
  braveWalletCryptoWalletsDescriptionOne: 'When you click Get started youll import your previous Crypto Wallet to the new Brave Wallet experience, and enjoy all the benefits outlined above. Give it a try, and let us know what you think!',
  braveWalletCryptoWalletsDescriptionTwo: 'If youd rather skip the import and keep the old Crypto Wallets experience, just navigate to the Brave Browser $1Settings$2 and change the default back to Crypto Wallets. You can also import, try the new Brave Wallet, and change back at any time.',
  braveWalletImportBraveLegacyDescription: 'Enter your existing crypto wallets password to import to Brave Wallet. Enjoy a faster and more secure way to manage crypto assets and interact with web 3 DApps.',
  braveWalletImportBraveLegacyInput: 'Crypto wallets password',
  braveWalletImportBraveLegacyAltButton: 'I’ve lost my password and recovery phrase, create a new wallet.',

  // Connect Hardware Wallet Panel
  braveWalletConnectHardwarePanelConnected: '$1 connected',
  braveWalletConnectHardwarePanelDisconnected: '$1 disconnected',
  braveWalletConnectHardwarePanelInstructions: 'Instructions',
  braveWalletConnectHardwarePanelConnect: 'Connect your $1',
  braveWalletConnectHardwarePanelConfirmation: 'Hardware wallet requires transaction confirmation. $1',

  // Transaction List Item
  braveWalletTransactionSent: 'sent',
  braveWalletTransactionReceived: 'received',
  braveWalletTransactionExplorerMissing: 'Block explorer URL is not available.',
  braveWalletTransactionExplorer: 'View on block explorer',
  braveWalletTransactionCopyHash: 'Copy transaction hash',
  braveWalletTransactionSpeedup: 'Speedup transaction',
  braveWalletTransactionCancel: 'Cancel transaction',
  braveWalletTransactionRetry: 'Retry transaction',
  braveWalletTransactionPlaceholder: 'Transactions will appear here',
  braveWalletTransactionApproveUnlimited: 'Unlimited',

  // Edit Gas
  braveWalletEditGasTitle1: 'Max priority fee',
  braveWalletEditGasTitle2: 'Edit gas',
  braveWalletEditGasDescription: 'While not a guarantee, miners will likely prioritize your transaction if you pay a higher fee.',
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
  braveWalletEditGasZeroGasPriceWarning: 'Transaction may not be propagated in the network.',

  // Advanced transaction settings
  braveWalletAdvancedTransactionSettings: 'Advanced settings',
  braveWalletAdvancedTransactionSettingsPlaceholder: 'Enter custom nonce value',
  braveWalletEditNonce: 'Nonce',
  braveWalletEditNonceInfo: 'The nonce value will be auto-determined if this field is not specified.',

  // Edit permissions
  braveWalletEditPermissionsTitle: 'Edit permissions',
  braveWalletEditPermissionsDescription: 'Spend limit permission allows $1 to withdraw and spend up to the following amount:',
  braveWalletEditPermissionsButton: 'Edit permissions',
  braveWalletEditPermissionsProposedAllowance: 'Proposed allowance',
  braveWalletEditPermissionsCustomAllowance: 'Custom allowance',

  // Send Input Errors
  braveWalletNotValidFilAddress: 'Not a valid FIL address',
  braveWalletNotValidEthAddress: 'Not a valid ETH address',
  braveWalletNotValidAddress: 'Not a valid address',
  braveWalletNotDomain: 'Domain is not registered',
  braveWalletSameAddressError: 'The receiving address is your own address',
  braveWalletContractAddressError: 'The receiving address is a tokens contract address',
  braveWalletAddressMissingChecksumInfoWarning: 'This address cannot be verified (missing checksum). Proceed?',
  braveWalletNotValidChecksumAddressError: 'Address did not pass verification (invalid checksum). Please try again, replacing lowercase letters with uppercase.',
  braveWalletMissingGasLimitError: 'Missing gas limit',
  braveWalletZeroBalanceError: 'Amount must be greater than 0',
  braveWalletAddressRequiredError: 'To address is required',

  // Transaction Queue Strings
  braveWalletQueueOf: 'of',
  braveWalletQueueNext: 'next',
  braveWalletQueueFirst: 'first',
  braveWalletQueueRejectAll: 'Reject transactions',

  // Add Suggested Token Panel
  braveWalletAddSuggestedTokenTitle: 'Add suggested token',
  braveWalletAddSuggestedTokenDescription: 'Would you like to import this token?',

  // Transaction Detail Panel
  braveWalletRecentTransactions: 'Recent transactions',
  braveWalletTransactionDetails: 'Transaction details',
  braveWalletTransactionDetailDate: 'Date',
  braveWalletTransactionDetailSpeedUp: 'Speedup',
  braveWalletTransactionDetailHash: 'Transaction hash',
  braveWalletTransactionDetailNetwork: 'Network',
  braveWalletTransactionDetailStatus: 'Status',

  // Transactions Status
  braveWalletTransactionStatusUnapproved: 'Unapproved',
  braveWalletTransactionStatusApproved: 'Approved',
  braveWalletTransactionStatusRejected: 'Rejected',
  braveWalletTransactionStatusSubmitted: 'Submitted',
  braveWalletTransactionStatusConfirmed: 'Confirmed',
  braveWalletTransactionStatusError: 'Error',

  // NFT Details Page
  braveWalletNFTDetailBlockchain: 'Blockchain',
  braveWalletNFTDetailTokenStandard: 'Token standard',
  braveWalletNFTDetailTokenID: 'Token ID',

  // Sweepstakes
  braveWalletSweepstakesTitle: 'Brave Swap-stakes',
  braveWalletSweepstakesDescription: '7 days of crypto giveaways, ~$500k in prizes.',
  braveWalletSweepstakesCallToAction: 'Enter now!',

  // Market Data Filters
  braveWalletMarketDataAllAssetsFilter: 'All Assets',
  braveWalletMarketDataTradableFilter: 'Tradable',
  braveWalletMarketDataAssetsColumn: 'Assets',
  braveWalletMarketDataPriceColumn: 'Price',
  braveWalletMarketData24HrColumn: '24Hr',
  braveWalletMarketDataMarketCapColumn: 'Mkt. Cap',
  braveWalletMarketDataVolumeColumn: 'Volume',

  // Network Filter
  braveWalletNetworkFilterAll: 'All Networks',
  braveWalletNetworkFilterSecondary: 'Secondary Networks'
})
