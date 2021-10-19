// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import struct Shared.Strings

extension Strings {
  struct Wallet {
    public static let portfolioPageTitle = NSLocalizedString(
      "wallet.portfolioPageTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Portfolio",
      comment: "The title of the portfolio page in the Crypto tab"
    )
    public static let accountsPageTitle = NSLocalizedString(
      "wallet.accountsPageTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Accounts",
      comment: "The title of the accounts page in the Crypto tab"
    )
    public static let selectedNetworkAccessibilityLabel = NSLocalizedString(
      "wallet.selectedNetwork",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Selected Network",
      comment: "The accessibility label for the ethereum network picker"
    )
    public static let assetsTitle = NSLocalizedString(
      "wallet.assetsTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Assets",
      comment: "The title which is displayed above a list of assets/tokens"
    )
    public static let transactionsTitle = NSLocalizedString(
      "wallet.transactionsTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Transactions",
      comment: "The title which is displayed above a list of transactions"
    )
    public static let assetSearchEmpty = NSLocalizedString(
      "wallet.assetSearchEmpty",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "No assets found",
      comment: "The text displayed when a user uses a query to search for assets that yields no results"
    )
    public static let searchTitle = NSLocalizedString(
      "wallet.searchTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Search assets",
      comment: "The title of the asset search page"
    )
    public static let noAssets = NSLocalizedString(
      "wallet.noAssets",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "No Assets",
      comment: "The empty state displayed when the user has no assets associated with an account"
    )
    public static let noAccounts = NSLocalizedString(
      "wallet.noAccounts",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "No Accounts",
      comment: "The empty state displayed when the user has no accounts associated with a transaction or asset"
    )
    public static let noTransactions = NSLocalizedString(
      "wallet.noTransactions",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "No Transactions",
      comment: "The empty state displayed when the user has no transactions associated with an account"
    )
    public static let detailsButtonTitle = NSLocalizedString(
      "wallet.detailsButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Details",
      comment: "A button title which when pressed displays a new screen with additional details/information"
    )
    public static let renameButtonTitle = NSLocalizedString(
      "wallet.renameButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Rename",
      comment: "A button on an account screen which when pressed presents a new screen to  rename the account"
    )
    public static let accountDetailsTitle = NSLocalizedString(
      "wallet.accountDetailsTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Account Details",
      comment: "The title displayed on the account details screen"
    )
    public static let accountDetailsNameTitle = NSLocalizedString(
      "wallet.accountDetailsNameTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Account Name",
      comment: "The title displayed above a text field that contains the account's name"
    )
    public static let accountDetailsNamePlaceholder = NSLocalizedString(
      "wallet.accountDetailsNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter account name",
      comment: "The placeholder of the text field which allows the user to edit the account's name"
    )
    public static let accountPrivateKey = NSLocalizedString(
      "wallet.accountPrivateKey",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Private Key",
      comment: "A button title for displaying their accounts private key"
    )
    public static let accountRemoveButtonTitle = NSLocalizedString(
      "wallet.accountRemoveButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Remove Account",
      comment: "A button title to trigger deleting a secondary account"
    )
    public static let accountRemoveAlertConfirmation = NSLocalizedString(
      "wallet.accountRemoveAlertConfirmation",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Remove this account?",
      comment: "The title of a confirmation dialog when attempting to remove an account"
    )
    public static let accountRemoveAlertConfirmationMessage = NSLocalizedString(
      "wallet.accountRemoveAlertConfirmationMessage",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Are you sure?",
      comment: "The message of a confirmation dialog when attempting to remove an account"
    )
    public static let accountPrivateKeyDisplayWarning = NSLocalizedString(
      "wallet.accountPrivateKeyDisplayWarning",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Warning: Never share your recovery phrase. Anyone with this phrase can take your assets forever.",
      comment: "A warning message displayed at the top of the Private Key screen"
    )
    public static let copyToPasteboard = NSLocalizedString(
      "wallet.copyToPasteboard",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Copy",
      comment: "A button title that when tapped will copy some data to the users clipboard"
    )
    public static let showPrivateKeyButtonTitle = NSLocalizedString(
      "wallet.showPrivateKeyButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Show Private Key",
      comment: "A button title that will make a private key visible on the screen"
    )
    public static let hidePrivateKeyButtonTitle = NSLocalizedString(
      "wallet.hidePrivateKeyButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Hide Private Key",
      comment: "A button title that will redact a private key on the screen"
    )
    public static let accountBackup = NSLocalizedString(
      "wallet.accountBackup",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Backup",
      comment: "A button title that shows a screen that allows the user to backup their recovery phrase"
    )
    public static let defaultAccountName = NSLocalizedString(
      "wallet.defaultAccountName",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Account %lld",
      comment: "The default account name when adding a primary account and not entering a custom name. '%lld' refers to a number (for example \"Account 3\")"
    )
    public static let defaultSecondaryAccountName = NSLocalizedString(
      "wallet.defaultSecondaryAccountName",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Secondary Account %lld",
      comment: "The default account name when adding a secondary account and not entering a custom name. '%lld' refers to a number (for example \"Secondary Account 3\")"
    )
    public static let addAccountTitle = NSLocalizedString(
      "wallet.addAccountTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Add Account",
      comment: "The title of the add account screen"
    )
    public static let addAccountAddButton = NSLocalizedString(
      "wallet.addAccountAddButton",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Add",
      comment: "The title of the button which when tapped will add a new account to the users list of crypto accounts"
    )
    public static let failedToImportAccountErrorTitle = NSLocalizedString(
      "wallet.failedToImportAccountErrorTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Failed to import account.",
      comment: "The title of an alert when the account the user attempted to import fails for some reason"
    )
    public static let failedToImportAccountErrorMessage = NSLocalizedString(
      "wallet.failedToImportAccountErrorMessage",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Please try again.",
      comment: "The message of an alert when the account the user attempted to import fails for some reason"
    )
    public static let importAccountOriginPasswordTitle = NSLocalizedString(
      "wallet.importAccountOriginPasswordTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Origin Password",
      comment: "A title above a text field that is used to enter a password"
    )
    public static let passwordPlaceholder = NSLocalizedString(
      "wallet.passwordPlaceholder",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Password",
      comment: "A placeholder string that will be used on password text fields"
    )
    public static let repeatedPasswordPlaceholder = NSLocalizedString(
      "wallet.repeatedPasswordPlaceholder",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Verify password",
      comment: "A placeholder string that will be used on repeat password text fields"
    )
    public static let importAccountSectionTitle = NSLocalizedString(
      "wallet.importAccountSectionTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "You can create a secondary account by importing your private key.",
      comment: "A title above a text field that will be used to import a users secondary accounts"
    )
    public static let importAccountPlaceholder = NSLocalizedString(
      "wallet.importAccountPlaceholder",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter, paste, or import your private key string file or JSON.",
      comment: "A placeholder on a text box for entering the users private key/json data to import accounts"
    )
    public static let importButtonTitle = NSLocalizedString(
      "wallet.importButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Import…",
      comment: "A button title that when tapped will display a file import dialog"
    )
    public static let primaryCryptoAccountsTitle = NSLocalizedString(
      "wallet.primaryCryptoAccountsTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Primary Crypto Accounts",
      comment: "A title above a list of crypto accounts that are not imported"
    )
    public static let secondaryCryptoAccountsTitle = NSLocalizedString(
      "wallet.secondaryCryptoAccountsTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Secondary Accounts",
      comment: "A title above a list of crypto accounts that are imported"
    )
    public static let secondaryCryptoAccountsSubtitle = NSLocalizedString(
      "wallet.secondaryCryptoAccountsSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Import your external wallet account with a separate seed phrase.",
      comment: "A subtitle above a list of crypto accounts that are imported"
    )
    public static let noSecondaryAccounts = NSLocalizedString(
      "wallet.noSecondaryAccounts",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "No secondary accounts.",
      comment: "The empty state shown when you have no imported accounts"
    )
    public static let incorrectPasswordErrorMessage = NSLocalizedString(
      "wallet.incorrectPasswordErrorMessage",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Incorrect password",
      comment: "The error message displayed when the user enters the wrong password while unlocking the wallet"
    )
    public static let unlockWalletTitle = NSLocalizedString(
      "wallet.unlockWalletTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter password to unlock wallet",
      comment: "The title displayed on the unlock wallet screen"
    )
    public static let unlockWalletButtonTitle = NSLocalizedString(
      "wallet.unlockWalletButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Unlock",
      comment: "The button title of the unlock wallet button. As in to enter a password and gain access to your wallet's assets."
    )
    public static let restoreWalletButtonTitle = NSLocalizedString(
      "wallet.restoreWalletButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Restore",
      comment: "The button title for showing the restore wallet screen. As in to use your recovery phrase to bring a wallet into Brave"
    )
    public static let cryptoTitle = NSLocalizedString(
      "wallet.cryptoTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Crypto",
      comment: "The title of the crypto tab"
    )
    public static let backupWalletWarningMessage = NSLocalizedString(
      "wallet.backupWalletWarningMessage",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Back up your wallet now to protect your crypto assets and ensure you never lose access.",
      comment: "The message displayed on the crypto tab if you have not yet completed the backup process"
    )
    public static let editVisibleAssetsButtonTitle = NSLocalizedString(
      "wallet.editVisibleAssetsButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Edit Visible Assets",
      comment: "The button title for showing the screen to change what assets are visible"
    )
    public static let buy = NSLocalizedString(
      "wallet.buy",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Buy",
      comment: "As in buying cryptocurrency"
    )
    public static let buyDescription = NSLocalizedString(
      "wallet.buyDescription",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Buy crypto with Apple Pay, credit or debit card.",
      comment: "The description of a buy button on the buy/send/swap modal"
    )
    public static let send = NSLocalizedString(
      "wallet.send",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Send",
      comment: "As in sending cryptocurrency to another account"
    )
    public static let sendDescription = NSLocalizedString(
      "wallet.sendDescription",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Send crypto or transfer from one account to another.",
      comment: "The description of a send button on the buy/send/swap modal"
    )
    public static let swap = NSLocalizedString(
      "wallet.swap",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Swap",
      comment: "As in swapping cryptocurrency from one asset to another"
    )
    public static let swapDescription = NSLocalizedString(
      "wallet.swapDescription",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Swap crypto assets with Brave DEX aggregator.",
      comment: "The description of a swap button on the buy/send/swap modal"
    )
    public static let infoTitle = NSLocalizedString(
      "wallet.infoTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Info",
      comment: "A title above additional information about an asset"
    )
    public static let verifyRecoveryPhraseTitle = NSLocalizedString(
      "wallet.verifyRecoveryPhraseTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Verify recovery phrase",
      comment: "The title of the verify recovery phrase screen"
    )
    public static let verifyRecoveryPhraseSubtitle = NSLocalizedString(
      "wallet.verifyRecoveryPhraseSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Select the words in your recovery phrase in their correct order.",
      comment: "The subtitle of the verify recovery phrase screen"
    )
    public static let verifyButtonTitle = NSLocalizedString(
      "wallet.verifyButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Verify",
      comment: "The button title to verify if the user has put all recovery words in the right order"
    )
    public static let skipButtonTitle = NSLocalizedString(
      "wallet.skipButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Skip",
      comment: "The button title to skip recovery phrase backup"
    )
    public static let backupWalletTitle = NSLocalizedString(
      "wallet.backupWalletTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Back up your crypto wallet",
      comment: "The title of the backup wallet screen"
    )
    public static let backupWalletSubtitle = NSLocalizedString(
      "wallet.backupWalletSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "In the next step you’ll see a 12-word recovery phrase, which you can use to recover your primary crypto accounts. Safe it someplace safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.",
      comment: "The subtitle of the backup wallet screen"
    )
    public static let backupWalletDisclaimer = NSLocalizedString(
      "wallet.backupWalletDisclaimer",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "I understand that if I lose my recovery phrase, I won’t be able to access my crypto wallet.",
      comment: "The label next to a toggle which the user must acknowledge"
    )
    public static let continueButtonTitle = NSLocalizedString(
      "wallet.continueButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Continue",
      comment: "A button title when a user will continue to the next step of something"
    )
    public static let backupWalletBackButtonTitle = NSLocalizedString(
      "wallet.backupWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Backup Wallet",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar"
    )
    public static let setupCryptoTitle = NSLocalizedString(
      "wallet.setupCryptoTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Welcome to the new Brave Wallet",
      comment: "The title displayed on the 'setup crypto' onboarding screen"
    )
    public static let setupCryptoSubtitle = NSLocalizedString(
      "wallet.setupCryptoSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Hold crypto assets in your custody. Track portfolio performance, and interact with web 3 DApps. Trade, invest, borrow, and lend with DeFi. All right from the Brave privacy browser.",
      comment: "The subtitle displayed on the 'setup crypto' onboarding screen"
    )
    public static let setupCryptoButtonTitle = NSLocalizedString(
      "wallet.setupCryptoButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Setup Crypto",
      comment: "The button title to continue to the next step on the 'setup crypto' screen. As in to begin the process of creating a wallet/setting up the cryptocurrency feature"
    )
    public static let setupCryptoButtonBackButtonTitle = NSLocalizedString(
      "wallet.setupCryptoButtonBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Welcome",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As in the first step of an onboarding process is to welcome a user."
    )
    public static let backupRecoveryPhraseTitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Your recovery phrase",
      comment: "The title of the backup recovery phrase screen"
    )
    public static let backupRecoveryPhraseSubtitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Write down or copy these words in the exact order shown below, and save them somewhere safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.",
      comment: "The subtitle of the backup recovery phrase screen"
    )
    public static let backupRecoveryPhraseDisclaimer = NSLocalizedString(
      "wallet.backupRecoveryPhraseDisclaimer",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "I have backed up my phrase somewhere safe.",
      comment: "The disclaimer next to a toggle that the user must acknowledge before proceeding"
    )
    public static let backupRecoveryPhraseWarningPartOne = NSLocalizedString(
      "wallet.backupRecoveryPhraseWarningPartOne",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "WARNING:",
      comment: "The first part of the warning displayed on the backup recovery phrase page. As in to pay attention to the following text"
    )
    public static let backupRecoveryPhraseWarningPartTwo = NSLocalizedString(
      "wallet.backupRecoveryPhraseWarningPartTwo",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Never share your recovery phrase. Anyone with this phrase can take your assets forever.",
      comment: "The second part of the warning displayed on the backup recovery phrase page."
    )
    public static let backupRecoveryPhraseBackButtonTitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Recovery Phrase",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As in the a list of words to recovery your account on another device/wallet"
    )
    public static let restoreWalletBackButtonTitle = NSLocalizedString(
      "wallet.restoreWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Restore Wallet",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As to gain access to your assets from a different device"
    )
    public static let restoreWalletPhraseInvalidError = NSLocalizedString(
      "wallet.restoreWalletPhraseInvalidError",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Invalid recovery phrase",
      comment: "The error message displayed when a user enters an invalid phrase to restore from. By phrase we mean 'recovery phrase' or 'recovery mnemonic'"
    )
    public static let passwordDoesNotMeetRequirementsError = NSLocalizedString(
      "wallet.passwordDoesNotMeetRequirementsError",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Passwords must be at least 7 characters, and contain at least one number and one special character.",
      comment: "The error message displayed when a user enters a password that does not meet the requirements"
    )
    public static let passwordsDontMatchError = NSLocalizedString(
      "wallet.passwordsDontMatchError",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Verified password doesn't match",
      comment: "The error displayed when entering two passwords that do not match that are expected to match"
    )
    public static let restoreWalletTitle = NSLocalizedString(
      "wallet.restoreWalletTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Restore Crypto Account",
      comment: "The title on the restore wallet screen."
    )
    public static let restoreWalletSubtitle = NSLocalizedString(
      "wallet.restoreWalletSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter your recovery phrase to restore your Brave wallet crypto account.",
      comment: "The subtitle on the restore wallet screen."
    )
    public static let restoreWalletPhrasePlaceholder = NSLocalizedString(
      "wallet.restoreWalletPhrasePlaceholder",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter your recovery phrase",
      comment: "The placeholder on the mneomic/recovery phrase text field"
    )
    public static let restoreWalletImportFromLegacyBraveWallet = NSLocalizedString(
      "wallet.restoreWalletImportFromLegacyBraveWallet",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Import from legacy Brave crypto wallets?",
      comment: "A toggle label to ask the user if their 24-word phrase is a legacy Brave crypto wallet"
    )
    public static let restoreWalletShowRecoveryPhrase = NSLocalizedString(
      "wallet.restoreWalletShowRecoveryPhrase",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Show Recovery Phrase",
      comment: "A toggle label that will enable or disable visibility of the contents in the recovery phrase text field"
    )
    public static let restoreWalletNewPasswordTitle = NSLocalizedString(
      "wallet.restoreWalletNewPasswordTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "New Password",
      comment: "A title displayed above 2 text fields for entering a new wallet password"
    )
    public static let createWalletBackButtonTitle = NSLocalizedString(
      "wallet.createWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Create Password",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As to make up a new password to create a wallet"
    )
    public static let createWalletTitle = NSLocalizedString(
      "wallet.createWalletTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Secure your crypto assets with a password",
      comment: "The title of the create wallet screen"
    )
    public static let biometricsSetupErrorTitle = NSLocalizedString(
      "wallet.biometricsSetupErrorTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Failed to enable biometrics unlocking.",
      comment: "The title of an alert when the user has an error setting up biometric unlock"
    )
    public static let biometricsSetupErrorMessage = NSLocalizedString(
      "wallet.biometricsSetupErrorMessage",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "There was an error while trying to enable biometrics unlocking. Please try again later.",
      comment: "The message of an alert when the user has an error setting up biometric unlock"
    )
    public static let settingsResetButtonTitle = NSLocalizedString(
      "wallet.settingsResetButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Reset",
      comment: "The title of a button that will reset the wallet. As in to erase the users wallet from the device"
    )
    public static let settingsResetWalletAlertTitle = NSLocalizedString(
      "wallet.settingsResetWalletAlertTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Are you sure you want to reset Brave Wallet?",
      comment: "The title the confirmation dialog when resetting the wallet. As in to erase the users wallet from the device"
    )
    public static let settingsResetWalletAlertMessage = NSLocalizedString(
      "wallet.settingsResetWalletAlertMessage",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "This action cannot be reversed. Your recovery phrase is the only way to regain account access to your crypto assets.",
      comment: "The message the confirmation dialog when resetting the wallet."
    )
    public static let settingsResetWalletAlertButtonTitle = NSLocalizedString(
      "wallet.settingsResetWalletAlertButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Reset Wallet",
      comment: "The title of a button that will reset the wallet. As in to erase the users wallet from the device"
    )
    public static let dateIntervalHour = NSLocalizedString(
      "wallet.dateIntervalHour",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1H",
      comment: "An abbreivated form of \"1 Hour\" used to describe what range of data to show on the graph (past hour)"
    )
    public static let dateIntervalHourAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalHourAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1 Hour",
      comment: "Describes what range of data to show on the graph (past hour)"
    )
    public static let dateIntervalDay = NSLocalizedString(
      "wallet.dateIntervalDay",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1D",
      comment: "An abbreivated form of \"1 Day\" used to describe what range of data to show on the graph (past day)"
    )
    public static let dateIntervalDayAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalDayAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1 Day",
      comment: "Describes what range of data to show on the graph (past day)"
    )
    public static let dateIntervalWeek = NSLocalizedString(
      "wallet.dateIntervalWeek",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1W",
      comment: "An abbreivated form of \"1 Week\" used to describe what range of data to show on the graph (past week)"
    )
    public static let dateIntervalWeekAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalWeekAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1 Week",
      comment: "Describes what range of data to show on the graph (past week)"
    )
    public static let dateIntervalMonth = NSLocalizedString(
      "wallet.dateIntervalMonth",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1M",
      comment: "An abbreivated form of \"1 Month\" used to describe what range of data to show on the graph (past month)"
    )
    public static let dateIntervalMonthAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalMonthAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1 Month",
      comment: "Describes what range of data to show on the graph (past month)"
    )
    public static let dateIntervalThreeMonths = NSLocalizedString(
      "wallet.dateIntervalThreeMonths",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "3M",
      comment: "An abbreivated form of \"3 Months\" used to describe what range of data to show on the graph (past 3 months)"
    )
    public static let dateIntervalThreeMonthsAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalThreeMonthsAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "3 Months",
      comment: "Describes what range of data to show on the graph (past 3 months)"
    )
    public static let dateIntervalYear = NSLocalizedString(
      "wallet.dateIntervalYear",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1Y",
      comment: "An abbreivated form of \"1 Year\" used to describe what range of data to show on the graph (past year)"
    )
    public static let dateIntervalYearAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalYearAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "1 Year",
      comment: "Describes what range of data to show on the graph (past year)"
    )
    public static let dateIntervalAll = NSLocalizedString(
      "wallet.dateIntervalAll",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "All",
      comment: "Describes what range of data to show on the graph (all data available)"
    )
    public static let swapCryptoFromTitle = NSLocalizedString(
      "wallet.swapCryptoFromTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "From",
      comment: "A title above the cryptocurrency token/asset you are swapping from. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let swapCryptoToTitle = NSLocalizedString(
      "wallet.swapCryptoToTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "To",
      comment: "A title above the cryptocurrency token/asset you are swapping to. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let swapCryptoAmountTitle = NSLocalizedString(
      "wallet.swapCryptoAmountTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter amount of %@ to swap",
      comment: "A title above the amount of asset you want to swap. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapCryptoAmountPlaceholder = NSLocalizedString(
      "wallet.swapCryptoAmountPlaceholder",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Amount in %@",
      comment: "A placeholder of the amount text field. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapCryptoAmountReceivingTitle = NSLocalizedString(
      "wallet.swapCryptoAmountReceivingTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Amount receiving in %@ (estimated)",
      comment: "A title above the amount of asset you will receive from the swap. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapOrderTypeLabel = NSLocalizedString(
      "wallet.swapOrderTypeLabel",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Order Type",
      comment: "The type of order you want to place. Options are: 'Market' and 'Limit'"
    )
    public static let swapLimitOrderType = NSLocalizedString(
      "wallet.swapLimitOrderType",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Limit",
      comment: "The 'Limit' order type. Limit orders only execute when the price requirements are met"
    )
    public static let swapMarketOrderType = NSLocalizedString(
      "wallet.swapMarketOrderType",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Market",
      comment: "The 'Market' order type. Market orders execute immediately based on the price at the time of the order."
    )
    public static let today = NSLocalizedString(
      "wallet.today",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Today",
      comment: "A label appended after a certain dollar or percent change. Example: 'Up 1.4% Today'"
    )
    public static let selectAccountTitle = NSLocalizedString(
      "wallet.selectAccountTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Select Account",
      comment: "The title of the account selection screen. Will show above a list of accounts the user may pick from"
    )
    public static let assetDetailSubtitle = NSLocalizedString(
      "wallet.assetDetailSubtitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "%@ Price (%@)",
      comment: "A subtitle on the asset details screen that uses the name and symbol. Example: Basic Attention Token Price (BAT)"
    )
    public static let biometricsSetupTitle = NSLocalizedString(
      "wallet.biometricsSetupTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Unlock Brave Wallet with your Face ID, Touch ID, or password.",
      comment: "The title shown when a user is asked if they would like to setup biometric unlock"
    )
    public static let biometricsSetupEnableButtonTitle = NSLocalizedString(
      "wallet.biometricsSetupEnableButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enable",
      comment: "The button title that enables the biometric unlock feature"
    )
    public static let copyAddressButtonTitle = NSLocalizedString(
      "wallet.copyAddressButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Copy Address",
      comment: "The button title that appears when long-pressing a wallet address that will copy said address to the users clipboard"
    )
    public static let autoLockTitle = NSLocalizedString(
      "wallet.autoLockTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Automatically Lock After",
      comment: "The title that appears before an auto-lock interval. Example: Automatically lock after 5 minutes"
    )
    public static let autoLockFooter = NSLocalizedString(
      "wallet.autoLockFooter",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "The number of minutes to wait until the Brave Wallet is automatically locked",
      comment: "The footer beneath the auto-lock title and interval duration"
    )
    public static let autoLockNeverInterval = NSLocalizedString(
      "wallet.autoLockNeverInterval",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Never",
      comment: "One of the auto-lock interval options. As in to never automatically lock the wallet."
    )
    public static let enterAmount = NSLocalizedString(
      "wallet.enterAmount",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Enter amount",
      comment: "The header title for the textField users will input the dollar value of the crypto they want to buy"
    )
    public static let amountInCurrency = NSLocalizedString(
      "wallet.amountInCurrency",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Amount in %@",
      comment: "The textField's placeholder for users to input dollar value of the crypto they want to buy. '%@' will be replaced with a currency such as 'USD' or 'EUR'"
    )
    public static let buyButtonTitle = NSLocalizedString(
      "wallet.buyButtonTitle",
      tableName: "BraveWallet",
      bundle: .braveWallet,
      value: "Continue to Wyre",
      comment: "The title of the button for users to click when they are ready to buy using Wyre payment"
    )
  }
}
