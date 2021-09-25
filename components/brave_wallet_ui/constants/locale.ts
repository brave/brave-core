const locale = {
  // App Categories
  defiCategory: 'Defi Apps',
  nftCategory: 'NFT Marketplaces',
  searchCategory: 'Search Results',

  // App Category Button Text
  defiButtonText: 'Browse More Defi',
  nftButtonText: 'Browse More NFT',

  // Compound App
  compoundName: 'Compound',
  compoundDescription: 'Unlock a universe of open financial applications.',
  compoundUrl: 'https://app.compound.finance/',

  // Maker App
  makerName: 'MakerDAO',
  makerDescription: 'Maker - stablecoin, loans and governance...',
  makerUrl: 'https://makerdao.com/en/',

  // Aave App
  aaveName: 'Aave',
  aaveDescription: 'Protocol to earn on deposits & borrow assets.',
  aaveUrl: 'https://aave.com/',

  // OpenSea App
  openSeaName: 'OpenSea',
  openSeaDescription: 'The largest NFT marketplace. Buy, sell, and discover rare digital items',
  openSeaUrl: 'https://opensea.io/',

  // Rarible App
  raribleName: 'Rarible',
  raribleDescription: 'Create and sell digital artworks',
  raribleUrl: 'https://rarible.com/',

  // Search Text
  searchText: 'Search',

  // Side Nav Buttons
  sideNavCrypto: 'Crypto',
  sideNavRewards: 'Rewards',
  sideNavCards: 'Cards',

  // Top Nav Tab Buttons
  topNavPortfolio: 'Portfolio',
  topTabPrices: 'Prices',
  topTabApps: 'Apps',
  topNavNFTS: 'NFTs',
  topNavAccounts: 'Accounts',

  // Chart Timeline Buttons
  chartLive: '1H',
  chartOneDay: '1D',
  chartOneWeek: '1W',
  chartOneMonth: '1M',
  chartThreeMonths: '3M',
  chartOneYear: '1Y',
  chartAllTime: 'All',

  // Portfolio View
  addCoin: 'Add Coin',
  balance: 'Balance',

  // Portfolio SubView
  accounts: 'Accounts',
  account: 'Account',
  transactions: 'Transactions',
  price: 'Price',
  back: 'Back',
  addAccount: 'Add Account',

  // BuySendSwap
  buy: 'Buy',
  send: 'Send',
  swap: 'Swap',
  bssToolTip: 'not supported',

  // Buttons
  buttonContinue: 'Continue',
  buttonBack: 'Back',
  buttonCopy: 'Copy',
  buttonVerify: 'Verify',

  // Wallet Onboarding Welcome
  welcomeTitle: 'DeFi & Secure Crypto Storage',
  welcomeDescription: 'Hold crypto in your custody. Trade assets, and interact with web 3 apps. Track portfolio performance, invest, borrow, and lend with DeFi.',
  welcomeButton: 'Get Started',
  welcomeRestoreButton: 'Restore',

  // Backup Wallet Intro
  backupIntroTitle: 'Backup your crypto account now!',
  backupIntroDescription: 'In the next step you will see 12 words that allows you to recover your crypto wallet.',
  backupIntroTerms: 'I understand that if I lose my recovery words, I will not be able to access my crypto wallet.',
  backupButtonSkip: 'Skip',
  backupButtonCancel: 'Cancel',

  // Recovery Phrase Backup
  recoveryTitle: 'Your recovery phrase',
  recoveryDescription: 'Write down or copy these words in the right order and save them somewhere safe.',
  recoveryWarning1: 'WARNING:',
  recoveryWarning2: 'Never disclose your backup phrase.',
  recoveryWarning3: 'Anyone with this phrase can take your funds forever.',
  recoveryTerms: 'I have backed up my phrase somewhere safe',

  // Verify Recovery Phrase
  verifyRecoveryTitle: 'Verify recovery phrase',
  verifyRecoveryDescription: 'Tap the words to put them next to each other in the correct order.',
  verifyError: 'Phrases did not match, please try again.',

  // Create Password
  createPasswordTitle: 'Secure your crypto with a password',
  createPasswordDescription: 'Passwords must be at least 7 characters containing at least one number and a special character.',
  createPasswordInput: 'Password',
  confirmPasswordInput: 'Re-Type Password',
  createPasswordError: 'Passwords must be at least 7 characters containing at least one number and a special character.',
  confirmPasswordError: 'Re-typed password does not match',

  // Lock Screen
  lockScreenTitle: 'Enter password to unlock wallet',
  lockScreenButton: 'Unlock',
  lockScreenError: 'Incorrect password',

  // Wallet More Popup
  walletPopupSettings: 'Settings',
  walletPopupLock: 'Lock Crypto',
  walletPopupBackup: 'Backup Wallet',

  // Backup Warning
  backupWarningText: 'Backup your wallet now to protect your crypto portfolio from loss of access.',
  backupButton: 'Backup',
  dismissButton: 'Dismiss',

  // Restore Screen
  restoreTite: 'Restore Primary Crypto Accounts',
  restoreDescription: 'Enter your recovery phrase to restore your Brave Wallet crypto account.',
  restoreError: 'The recovery phrase entered is invalid.',
  restorePlaceholder: 'Paste recovery phrase from clipboard',
  restoreShowPhrase: 'Show recovery phrase',
  restoreLegacyCheckBox: 'Import from legacy Brave Crypto Wallet?',
  restoreFormText: 'New Password',

  // Tool Tips
  toolTipCopyToClipboard: 'Copy to Clipboard',

  // Accounts Tab
  accountsPrimary: 'Primary Crypto Accounts',
  accountsSecondary: 'Secondary Accounts',
  accountsSecondaryDisclaimer: 'These accounts can be used with web 3 dapps and can be shown in your portfolio, however, they cannot not be restored via recovery phrase from your primary account backup. Enable Brave Sync accounts to auto-restore secondary accounts.',
  accountsAssets: 'Assets',
  accountsEditVisibleAssets: 'Visible assets',

  // Add Account Options
  addAccountCreate: 'Create',
  addAccountImport: 'Import',
  addAccountHardware: 'Hardware',
  addAccountConnect: 'Connect',
  addAccountPlaceholder: 'Account Name',

  // Import Account
  importAccountButton: 'Import',
  importAccountDisclaimer: 'Imported accounts can be used with web 3 dapps and can be shown in your portfolio, however, they cannot not be restored via recovery phrase from your primary account backup. Enable Brave Sync accounts to auto-restore secondary accounts.',
  importAccountPlaceholder: 'Paste private key from clipboard',
  importAccountKey: 'Private Key',
  importAccountFile: 'JSON File',
  importAccountUploadButton: 'Choose File',
  importAccountUploadPlaceholder: 'No file chosen',
  importAccountError: 'Failed to import account, please try again.',

  // Connect Hardware Wallet
  connectHardwareTitle: 'Select your hardware wallet device',
  connectHardwareInfo1: 'Connect your',
  connectHardwareInfo2: 'wallet directly to your computer.',
  connectHardwareInfo3: 'Unlock your device and select the Ethereum app.',
  connectHardwareTrezor: 'Trezor',
  connectHardwareLedger: 'Ledger',
  connectingHardwareWallet: 'Connecting...',
  addCheckedAccountsHardwareWallet: 'Add Checked Accounts',
  loadMoreAccountsHardwareWallet: 'Load more',
  searchScannedAccounts: 'Search account 0x',
  switchHDPathTextHardwareWallet: 'Try switching HD path (above) if you cannot find the account you are looking for.',
  ledgerLiveDerivationPath: 'Ledger Live',
  ledgerLegacyDerivationPath: 'Legacy (MEW/MyCrypto)',
  unknownInternalError: 'Unknown error, please reconnect device and try again',

  // Account Settings Modal
  accountSettingsDetails: 'Details',
  accountSettingsWatchlist: 'Visible Assets',
  accountSettingsPrivateKey: 'Private Key',
  accountSettingsSave: 'Save',
  accountSettingsRemove: 'Remove Account',
  watchlistAddCustomAsset: 'Add custom asset',
  watchListTokenName: 'Token name',
  watchListTokenAddress: 'Token contract address',
  watchListTokenSymbol: 'Token symbol',
  watchListTokenDecimals: 'Decimals of Percision',
  watchListAdd: 'Add',
  watchListSuggestion: 'as a custtom token',
  watchListNoAsset: 'No assets named',
  watchListSearchPlaceholder: 'Search tokens or contract address',
  watchListError: 'Failed to add custom token, please try again.',
  accountSettingsDisclaimer: 'Warning: Never disclose this key. Anyone with your private key can steal any assets held in your account.',
  accountSettingsShowKey: 'Show Key',
  accountSettingsHideKey: 'Hide Key',
  accountSettingsUpdateError: 'Failed to update account name, please try again.',

  // AmountPresets
  preset25: '25%',
  preset50: '50%',
  preset75: '75%',
  preset100: 'All',

  // Networks
  networkETH: 'Ethereum',
  networkMain: 'Mainnet',
  networkTest: 'Test Network',
  networkRopsten: 'Roptsten',
  networkKovan: 'Kovan',
  networkRinkeby: 'Rinkeby',
  networkGoerli: 'Goerli',
  networkBinance: 'Binance Smart Chain',
  networkBinanceAbbr: 'BSC',
  networkLocalhost: 'Localhost',

  // Select Screens
  selectAccount: 'Select Account',
  searchAccount: 'Search Accounts',
  selectNetwork: 'Select Network',
  selectAsset: 'Select From',
  searchAsset: 'Search Coins',

  // Swap
  swapFrom: 'Amount',
  swapTo: 'To',
  swapEstimate: 'estimate',
  swapMarket: 'Market',
  swapLimit: 'Limit',
  swapPriceIn: 'Price in',

  // Buy
  buyTitle: 'Test Faucet',
  buyDescription: 'Get Ether from a faucet for',
  buyWyreButton: 'Continue to Wyre',
  buyFaucetButton: 'Get Ether',

  // Sign Transaction Panel
  signTransactionTitle: 'Your signature is being requested',
  braveWalletSignWarning: 'Note that Brave can’t verify what will happen if you sign. A signature could authorize nearly any operation in your account or on your behalf, including (but not limited to) giving total control of your account and crypto assets to the site making the request. Only sign if you’re sure you want to take this action, and trust the requesting site.',
  braveWalletSignWarningTitle: 'Sign at your own risk',
  signTransactionMessageTitle: 'Message',
  signTransactionButton: 'Sign',

  // Allow Spend ERC20 Panel
  allowSpendTitle: 'Allow this app to spend your',
  allowSpendDescriptionFirstHalf: 'By granting this permission, you are allowing this app to withdraw your ',
  allowSpendDescriptionSecondHalf: ' and automate transactions for you.',
  allowSpendBoxTitle: 'Edit Permissions',
  allowSpendTransactionFee: 'Transaction Fee',
  allowSpendEditButton: 'Edit',
  allowSpendDetailsButton: 'View details',
  allowSpendRejectButton: 'Reject',
  allowSpendConfirmButton: 'Confirm',

  // Allow Add Network Panel
  allowAddNetworkTitle: 'Allow this site to add a network?',
  allowAddNetworkDescription: 'This will allow this network to be used within Brave Wallet.',
  allowAddNetworkLearnMoreButton: 'Learn More.',
  allowAddNetworkName: 'Network Name',
  allowAddNetworkUrl: 'Network URL',
  allowAddNetworkDetailsButton: 'View all details',
  allowAddNetworkApproveButton: 'Approve',
  allowAddNetworkChainID: 'Chain ID',
  allowAddNetworkCurrencySymbol: 'Currency Symbol',
  allowAddNetworkExplorer: 'Block Explorer URL',

  // Confirm Transaction Panel
  confirmTransactionTotal: 'Total',
  confirmTransactionGasFee: 'Gas Fee',
  confrimTransactionBid: 'Bid',
  confirmTransactionAmountGas: 'Amount + Gas',
  confirmTransactionNoData: 'No Data.',

  // Wallet Main Panel
  panelTitle: 'Brave Wallet',
  panelConnected: 'Connected',
  panelNotConnected: 'Not Connected',

  // Transaction Detail Box
  transactionDetailBoxFunction: 'FUNCTION TYPE',
  transactionDetailBoxHex: 'HEX DATA',
  transactionDetailBoxBytes: 'BYTES',

  // Connect With Site Panel
  connectWithSiteTitle: 'Select accounts(s)',
  connectWithSiteDescription1: 'View the addressess of your',
  connectWithSiteDescription2: 'permitted accounts (required)',
  connectWithSiteNext: 'Next',
  connectWithSiteHeaderTitle: 'Connect With Brave Wallet',

  // Import from external wallets
  braveWalletImportFromExternalNewPassword: 'A more secure password is required for Brave Wallet',
  braveWalletImportFromExternalCreatePassword: 'Set a Brave Wallet password',
  braveWalletImportFromExternalPasswordCheck: 'Use the same password',

  // Import from MetaMask
  importMetaMaskTitle: 'Import from MetaMask',
  importMetaMaskDescription: 'Import your MetaMask HD wallet, assets, and settings, into Brave Wallet. Enjoy a faster and more secure way to manage crypto, interact with DeFi. With more assets support like Doge, Bitcoin, Solana.',
  importMetaMaskInput: 'Enter your MetaMask password',

  // Import from Legacy Wallet
  importBraveLegacyTitle: 'Existing Crypto Wallets detected',
  importBraveLegacyDescription: 'Enter your existing crypto wallet password to update. Enjoy a faster and more secure way to manage crypto, interact with DeFi. With more assets support like Doge, Bitcoin, Solana.',
  importBraveLegacyInput: 'Enter your crypto wallets password',
  importBraveLegacyAltButton: 'I’ve lost my password and recovery phrase',

  // Connect Hardware Wallet Panel
  connectHardwarePanelConnected: 'connected',
  connectHardwarePanelDisconnected: 'disconnected',
  connectHardwarePanelInstructions: 'Instructions',
  connectHardwarePanelConnect: 'Connect your',
  connectHardwarePanelConfirmation: 'Device requires transaction confirmation',

  // Transaction List Item
  transactionSent: 'sent',
  transactionReceived: 'received',
  transactionExplorerMissing: 'No block explorer url defined.',
  transactionExplorer: 'View on Block Explorer',
  transactionPlaceholder: 'Transactions will appear here'
}

export default locale
