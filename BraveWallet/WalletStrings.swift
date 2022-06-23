// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct Wallet {
    public static let braveWallet = NSLocalizedString(
      "wallet.module",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Brave Wallet",
      comment: "The title shown on the wallet settings page, and the value shown when selecting the default wallet as Brave Wallet in wallet settings."
    )
    public static let wallet = NSLocalizedString(
      "wallet.wallet",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Wallet",
      comment: "The title shown on the menu to access Brave Wallet"
    )
    public static let otherWalletActionsAccessibilityTitle = NSLocalizedString(
      "wallet.otherWalletActionsAccessibilityTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Other wallet actions",
      comment: "The label read out when a user is using VoiceOver and highlights the ellipsis button on the portfolio page"
    )
    public static let portfolioPageTitle = NSLocalizedString(
      "wallet.portfolioPageTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Portfolio",
      comment: "The title of the portfolio page in the Crypto tab"
    )
    public static let accountsPageTitle = NSLocalizedString(
      "wallet.accountsPageTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Accounts",
      comment: "The title of the accounts page in the Crypto tab"
    )
    public static let selectedNetworkAccessibilityLabel = NSLocalizedString(
      "wallet.selectedNetwork",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Selected Network",
      comment: "The accessibility label for the ethereum network picker"
    )
    public static let selectedAccountAccessibilityLabel = NSLocalizedString(
      "wallet.selectedAccount",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Selected Account",
      comment: "The accessibility label for the selected account picker"
    )
    public static let assetsTitle = NSLocalizedString(
      "wallet.assetsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Assets",
      comment: "The title which is displayed above a list of assets/tokens"
    )
    public static let transactionsTitle = NSLocalizedString(
      "wallet.transactionsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transactions",
      comment: "The title which is displayed above a list of transactions"
    )
    public static let assetSearchEmpty = NSLocalizedString(
      "wallet.assetSearchEmpty",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No assets found",
      comment: "The text displayed when a user uses a query to search for assets that yields no results"
    )
    public static let searchTitle = NSLocalizedString(
      "wallet.searchTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Search assets",
      comment: "The title of the asset search page"
    )
    public static let noAssets = NSLocalizedString(
      "wallet.noAssets",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No Assets",
      comment: "The empty state displayed when the user has no assets associated with an account"
    )
    public static let noAccounts = NSLocalizedString(
      "wallet.noAccounts",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No Accounts",
      comment: "The empty state displayed when the user has no accounts associated with a transaction or asset"
    )
    public static let noTransactions = NSLocalizedString(
      "wallet.noTransactions",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No Transactions",
      comment: "The empty state displayed when the user has no transactions associated with an account"
    )
    public static let detailsButtonTitle = NSLocalizedString(
      "wallet.detailsButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Details",
      comment: "A button title which when pressed displays a new screen with additional details/information"
    )
    public static let renameButtonTitle = NSLocalizedString(
      "wallet.renameButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Rename",
      comment: "A button on an account screen which when pressed presents a new screen to  rename the account"
    )
    public static let accountDetailsTitle = NSLocalizedString(
      "wallet.accountDetailsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Account Details",
      comment: "The title displayed on the account details screen"
    )
    public static let accountDetailsNameTitle = NSLocalizedString(
      "wallet.accountDetailsNameTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Account Name",
      comment: "The title displayed above a text field that contains the account's name"
    )
    public static let accountDetailsNamePlaceholder = NSLocalizedString(
      "wallet.accountDetailsNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter account name",
      comment: "The placeholder of the text field which allows the user to edit the account's name"
    )
    public static let accountPrivateKey = NSLocalizedString(
      "wallet.accountPrivateKey",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Private Key",
      comment: "A button title for displaying their accounts private key"
    )
    public static let accountRemoveButtonTitle = NSLocalizedString(
      "wallet.accountRemoveButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Remove Account",
      comment: "A button title to trigger deleting a secondary account"
    )
    public static let accountRemoveAlertConfirmation = NSLocalizedString(
      "wallet.accountRemoveAlertConfirmation",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Remove this account?",
      comment: "The title of a confirmation dialog when attempting to remove an account"
    )
    public static let accountRemoveAlertConfirmationMessage = NSLocalizedString(
      "wallet.accountRemoveAlertConfirmationMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Are you sure?",
      comment: "The message of a confirmation dialog when attempting to remove an account"
    )
    public static let accountPrivateKeyDisplayWarning = NSLocalizedString(
      "wallet.accountPrivateKeyDisplayWarning",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Warning: Never share your private key. Anyone with this key can take your assets forever.",
      comment: "A warning message displayed at the top of the Private Key screen"
    )
    public static let copyToPasteboard = NSLocalizedString(
      "wallet.copyToPasteboard",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Copy",
      comment: "A button title that when tapped will copy some data to the users clipboard"
    )
    public static let pasteFromPasteboard = NSLocalizedString(
      "wallet.pasteFromPasteboard",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Paste",
      comment: "A button title that when tapped will paste some data from the users clipboard to a text field"
    )
    public static let showPrivateKeyButtonTitle = NSLocalizedString(
      "wallet.showPrivateKeyButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Show Private Key",
      comment: "A button title that will make a private key visible on the screen"
    )
    public static let hidePrivateKeyButtonTitle = NSLocalizedString(
      "wallet.hidePrivateKeyButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Hide Private Key",
      comment: "A button title that will redact a private key on the screen"
    )
    public static let accountBackup = NSLocalizedString(
      "wallet.accountBackup",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Backup",
      comment: "A button title that shows a screen that allows the user to backup their recovery phrase"
    )
    public static let defaultAccountName = NSLocalizedString(
      "wallet.defaultAccountName",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Account %lld",
      comment: "The default account name when adding a primary account and not entering a custom name. '%lld' refers to a number (for example \"Account 3\")"
    )
    public static let defaultSecondaryAccountName = NSLocalizedString(
      "wallet.defaultSecondaryAccountName",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Secondary Account %lld",
      comment: "The default account name when adding a secondary account and not entering a custom name. '%lld' refers to a number (for example \"Secondary Account 3\")"
    )
    public static let addAccountTitle = NSLocalizedString(
      "wallet.addAccountTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add Account",
      comment: "The title of the add account screen"
    )
    public static let add = NSLocalizedString(
      "wallet.addAccountAddButton",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add",
      comment: "The title of the button which when tapped will add a new account to the users list of crypto accounts. It will be used as the title of the navigation bar item button on the top right of the add custom token scree."
    )
    public static let failedToImportAccountErrorTitle = NSLocalizedString(
      "wallet.failedToImportAccountErrorTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Failed to import account.",
      comment: "The title of an alert when the account the user attempted to import fails for some reason"
    )
    public static let failedToImportAccountErrorMessage = NSLocalizedString(
      "wallet.failedToImportAccountErrorMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Please try again.",
      comment: "The message of an alert when the account the user attempted to import fails for some reason"
    )
    public static let importAccountOriginPasswordTitle = NSLocalizedString(
      "wallet.importAccountOriginPasswordTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Origin Password",
      comment: "A title above a text field that is used to enter a password"
    )
    public static let passwordPlaceholder = NSLocalizedString(
      "wallet.passwordPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Password",
      comment: "A placeholder string that will be used on password text fields"
    )
    public static let repeatedPasswordPlaceholder = NSLocalizedString(
      "wallet.repeatedPasswordPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Verify password",
      comment: "A placeholder string that will be used on repeat password text fields"
    )
    public static let importAccountSectionTitle = NSLocalizedString(
      "wallet.importAccountSectionTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "You can create a secondary account by importing your private key.",
      comment: "A title above a text field that will be used to import a users secondary accounts"
    )
    public static let importAccountPlaceholder = NSLocalizedString(
      "wallet.importAccountPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter, paste, or import your private key string file or JSON.",
      comment: "A placeholder on a text box for entering the users private key/json data to import accounts"
    )
    public static let importButtonTitle = NSLocalizedString(
      "wallet.importButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Import…",
      comment: "A button title that when tapped will display a file import dialog"
    )
    public static let primaryCryptoAccountsTitle = NSLocalizedString(
      "wallet.primaryCryptoAccountsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Primary Crypto Accounts",
      comment: "A title above a list of crypto accounts that are not imported"
    )
    public static let secondaryCryptoAccountsTitle = NSLocalizedString(
      "wallet.secondaryCryptoAccountsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Secondary Accounts",
      comment: "A title above a list of crypto accounts that are imported"
    )
    public static let secondaryCryptoAccountsSubtitle = NSLocalizedString(
      "wallet.secondaryCryptoAccountsSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Import your external wallet account with a separate seed phrase.",
      comment: "A subtitle above a list of crypto accounts that are imported"
    )
    public static let noSecondaryAccounts = NSLocalizedString(
      "wallet.noSecondaryAccounts",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No secondary accounts.",
      comment: "The empty state shown when you have no imported accounts"
    )
    public static let incorrectPasswordErrorMessage = NSLocalizedString(
      "wallet.incorrectPasswordErrorMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Incorrect password",
      comment: "The error message displayed when the user enters the wrong password while unlocking the wallet"
    )
    public static let unlockWalletTitle = NSLocalizedString(
      "wallet.unlockWalletTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter password to unlock wallet",
      comment: "The title displayed on the unlock wallet screen"
    )
    public static let unlockWalletButtonTitle = NSLocalizedString(
      "wallet.unlockWalletButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unlock",
      comment: "The button title of the unlock wallet button. As in to enter a password and gain access to your wallet's assets."
    )
    public static let restoreWalletButtonTitle = NSLocalizedString(
      "wallet.restoreWalletButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Restore",
      comment: "The button title for showing the restore wallet screen. As in to use your recovery phrase to bring a wallet into Brave"
    )
    public static let cryptoTitle = NSLocalizedString(
      "wallet.cryptoTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Brave Wallet",
      comment: "The title of the crypto tab"
    )
    public static let backupWalletWarningMessage = NSLocalizedString(
      "wallet.backupWalletWarningMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Back up your wallet now to protect your crypto assets and ensure you never lose access.",
      comment: "The message displayed on the crypto tab if you have not yet completed the backup process"
    )
    public static let editVisibleAssetsButtonTitle = NSLocalizedString(
      "wallet.editVisibleAssetsButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Edit Visible Assets",
      comment: "The button title for showing the screen to change what assets are visible"
    )
    public static let buy = NSLocalizedString(
      "wallet.buy",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Buy",
      comment: "As in buying cryptocurrency"
    )
    public static let buyDescription = NSLocalizedString(
      "wallet.buyDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Buy crypto with Apple Pay, credit or debit card through Wyre.",
      comment: "The description of a buy button on the buy/send/swap modal"
    )
    public static let send = NSLocalizedString(
      "wallet.send",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Send",
      comment: "As in sending cryptocurrency to another account"
    )
    public static let sendDescription = NSLocalizedString(
      "wallet.sendDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Send crypto or transfer from one account to another.",
      comment: "The description of a send button on the buy/send/swap modal"
    )
    public static let swap = NSLocalizedString(
      "wallet.swap",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Swap",
      comment: "As in swapping cryptocurrency from one asset to another"
    )
    public static let swapDescription = NSLocalizedString(
      "wallet.swapDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Swap tokens and assets.",
      comment: "The description of a swap button on the buy/send/swap modal"
    )
    public static let infoTitle = NSLocalizedString(
      "wallet.infoTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Info",
      comment: "A title above additional information about an asset"
    )
    public static let verifyRecoveryPhraseTitle = NSLocalizedString(
      "wallet.verifyRecoveryPhraseTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Verify recovery phrase",
      comment: "The title of the verify recovery phrase screen"
    )
    public static let verifyRecoveryPhraseSubtitle = NSLocalizedString(
      "wallet.verifyRecoveryPhraseSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Select the words in your recovery phrase in their correct order.",
      comment: "The subtitle of the verify recovery phrase screen"
    )
    public static let verifyButtonTitle = NSLocalizedString(
      "wallet.verifyButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Verify",
      comment: "The button title to verify if the user has put all recovery words in the right order"
    )
    public static let skipButtonTitle = NSLocalizedString(
      "wallet.skipButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Skip",
      comment: "The button title to skip recovery phrase backup"
    )
    public static let backupWalletTitle = NSLocalizedString(
      "wallet.backupWalletTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Back up your crypto wallet",
      comment: "The title of the backup wallet screen"
    )
    public static let backupWalletSubtitle = NSLocalizedString(
      "wallet.backupWalletSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "In the next step you’ll see a 12-word recovery phrase, which you can use to recover your primary crypto accounts. Save it someplace safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.",
      comment: "The subtitle of the backup wallet screen"
    )
    public static let backupWalletDisclaimer = NSLocalizedString(
      "wallet.backupWalletDisclaimer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "I understand that if I lose my recovery phrase, I won’t be able to access my crypto wallet.",
      comment: "The label next to a toggle which the user must acknowledge"
    )
    public static let continueButtonTitle = NSLocalizedString(
      "wallet.continueButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Continue",
      comment: "A button title when a user will continue to the next step of something"
    )
    public static let backupWalletBackButtonTitle = NSLocalizedString(
      "wallet.backupWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Backup Wallet",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar"
    )
    public static let setupCryptoTitle = NSLocalizedString(
      "wallet.setupCryptoTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Welcome to the new Brave Wallet",
      comment: "The title displayed on the 'setup crypto' onboarding screen"
    )
    public static let setupCryptoSubtitle = NSLocalizedString(
      "wallet.setupCryptoSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Hold crypto assets in your custody and track portfolio performance. All right from the Brave privacy browser.",
      comment: "The subtitle displayed on the 'setup crypto' onboarding screen"
    )
    public static let setupCryptoButtonTitle = NSLocalizedString(
      "wallet.setupCryptoButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Get Started",
      comment: "The button title to continue to the next step on the 'setup crypto' screen. As in to begin the process of creating a wallet/setting up the cryptocurrency feature"
    )
    public static let setupCryptoButtonBackButtonTitle = NSLocalizedString(
      "wallet.setupCryptoButtonBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Welcome",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As in the first step of an onboarding process is to welcome a user."
    )
    public static let backupRecoveryPhraseTitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Your recovery phrase",
      comment: "The title of the backup recovery phrase screen"
    )
    public static let backupRecoveryPhraseSubtitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Write down or copy these words in the exact order shown below, and save them somewhere safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.",
      comment: "The subtitle of the backup recovery phrase screen"
    )
    public static let backupRecoveryPhraseDisclaimer = NSLocalizedString(
      "wallet.backupRecoveryPhraseDisclaimer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "I have backed up my phrase somewhere safe.",
      comment: "The disclaimer next to a toggle that the user must acknowledge before proceeding"
    )
    public static let backupRecoveryPhraseWarningPartOne = NSLocalizedString(
      "wallet.backupRecoveryPhraseWarningPartOne",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "WARNING:",
      comment: "The first part of the warning displayed on the backup recovery phrase page. As in to pay attention to the following text"
    )
    public static let backupRecoveryPhraseWarningPartTwo = NSLocalizedString(
      "wallet.backupRecoveryPhraseWarningPartTwo",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Never share your recovery phrase. Anyone with this phrase can take your assets forever.",
      comment: "The second part of the warning displayed on the backup recovery phrase page."
    )
    public static let backupRecoveryPhraseBackButtonTitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Recovery Phrase",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As in the a list of words to recovery your account on another device/wallet"
    )
    public static let restoreWalletBackButtonTitle = NSLocalizedString(
      "wallet.restoreWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Restore Wallet",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As to gain access to your assets from a different device"
    )
    public static let restoreWalletPhraseInvalidError = NSLocalizedString(
      "wallet.restoreWalletPhraseInvalidError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Invalid recovery phrase",
      comment: "The error message displayed when a user enters an invalid phrase to restore from. By phrase we mean 'recovery phrase' or 'recovery mnemonic'"
    )
    public static let passwordDoesNotMeetRequirementsError = NSLocalizedString(
      "wallet.passwordDoesNotMeetRequirementsError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Passwords must be at least 7 characters, and contain at least one letter, one number, and one special character.",
      comment: "The error message displayed when a user enters a password that does not meet the requirements"
    )
    public static let passwordsDontMatchError = NSLocalizedString(
      "wallet.passwordsDontMatchError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Verified password doesn't match",
      comment: "The error displayed when entering two passwords that do not match that are expected to match"
    )
    public static let restoreWalletTitle = NSLocalizedString(
      "wallet.restoreWalletTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Restore Crypto Account",
      comment: "The title on the restore wallet screen."
    )
    public static let restoreWalletSubtitle = NSLocalizedString(
      "wallet.restoreWalletSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter your recovery phrase to restore your Brave wallet crypto account.",
      comment: "The subtitle on the restore wallet screen."
    )
    public static let restoreWalletPhrasePlaceholder = NSLocalizedString(
      "wallet.restoreWalletPhrasePlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter your recovery phrase",
      comment: "The placeholder on the mneomic/recovery phrase text field"
    )
    public static let restoreWalletImportFromLegacyBraveWallet = NSLocalizedString(
      "wallet.restoreWalletImportFromLegacyBraveWallet",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Import from legacy Brave crypto wallets?",
      comment: "A toggle label to ask the user if their 24-word phrase is a legacy Brave crypto wallet"
    )
    public static let restoreWalletShowRecoveryPhrase = NSLocalizedString(
      "wallet.restoreWalletShowRecoveryPhrase",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Show Recovery Phrase",
      comment: "A toggle label that will enable or disable visibility of the contents in the recovery phrase text field"
    )
    public static let restoreWalletNewPasswordTitle = NSLocalizedString(
      "wallet.restoreWalletNewPasswordTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "New Password",
      comment: "A title displayed above 2 text fields for entering a new wallet password"
    )
    public static let createWalletBackButtonTitle = NSLocalizedString(
      "wallet.createWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Create Password",
      comment: "The title that will be displayed when long-pressing the back button in the navigation bar. As to make up a new password to create a wallet"
    )
    public static let createWalletTitle = NSLocalizedString(
      "wallet.createWalletTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Secure your crypto assets with a password",
      comment: "The title of the create wallet screen"
    )
    public static let biometricsSetupErrorTitle = NSLocalizedString(
      "wallet.biometricsSetupErrorTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Failed to enable biometrics unlocking.",
      comment: "The title of an alert when the user has an error setting up biometric unlock"
    )
    public static let biometricsSetupErrorMessage = NSLocalizedString(
      "wallet.biometricsSetupErrorMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "There was an error while trying to enable biometrics unlocking. Please try again later.",
      comment: "The message of an alert when the user has an error setting up biometric unlock"
    )
    public static let settingsResetButtonTitle = NSLocalizedString(
      "wallet.settingsResetButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Reset",
      comment: "The title of a button that will reset the wallet. As in to erase the users wallet from the device"
    )
    public static let settingsResetWalletAlertTitle = NSLocalizedString(
      "wallet.settingsResetWalletAlertTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Are you sure you want to reset Brave Wallet?",
      comment: "The title the confirmation dialog when resetting the wallet. As in to erase the users wallet from the device"
    )
    public static let settingsResetWalletAlertMessage = NSLocalizedString(
      "wallet.settingsResetWalletAlertMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This action cannot be reversed. Your recovery phrase is the only way to regain account access to your crypto assets.",
      comment: "The message the confirmation dialog when resetting the wallet."
    )
    public static let settingsResetWalletAlertButtonTitle = NSLocalizedString(
      "wallet.settingsResetWalletAlertButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Reset Wallet",
      comment: "The title of a button that will reset the wallet. As in to erase the users wallet from the device"
    )
    public static let dateIntervalHour = NSLocalizedString(
      "wallet.dateIntervalHour",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1H",
      comment: "An abbreivated form of \"1 Hour\" used to describe what range of data to show on the graph (past hour)"
    )
    public static let dateIntervalHourAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalHourAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1 Hour",
      comment: "Describes what range of data to show on the graph (past hour)"
    )
    public static let dateIntervalDay = NSLocalizedString(
      "wallet.dateIntervalDay",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1D",
      comment: "An abbreivated form of \"1 Day\" used to describe what range of data to show on the graph (past day)"
    )
    public static let dateIntervalDayAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalDayAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1 Day",
      comment: "Describes what range of data to show on the graph (past day)"
    )
    public static let dateIntervalWeek = NSLocalizedString(
      "wallet.dateIntervalWeek",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1W",
      comment: "An abbreivated form of \"1 Week\" used to describe what range of data to show on the graph (past week)"
    )
    public static let dateIntervalWeekAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalWeekAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1 Week",
      comment: "Describes what range of data to show on the graph (past week)"
    )
    public static let dateIntervalMonth = NSLocalizedString(
      "wallet.dateIntervalMonth",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1M",
      comment: "An abbreivated form of \"1 Month\" used to describe what range of data to show on the graph (past month)"
    )
    public static let dateIntervalMonthAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalMonthAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1 Month",
      comment: "Describes what range of data to show on the graph (past month)"
    )
    public static let dateIntervalThreeMonths = NSLocalizedString(
      "wallet.dateIntervalThreeMonths",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "3M",
      comment: "An abbreivated form of \"3 Months\" used to describe what range of data to show on the graph (past 3 months)"
    )
    public static let dateIntervalThreeMonthsAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalThreeMonthsAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "3 Months",
      comment: "Describes what range of data to show on the graph (past 3 months)"
    )
    public static let dateIntervalYear = NSLocalizedString(
      "wallet.dateIntervalYear",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1Y",
      comment: "An abbreivated form of \"1 Year\" used to describe what range of data to show on the graph (past year)"
    )
    public static let dateIntervalYearAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalYearAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "1 Year",
      comment: "Describes what range of data to show on the graph (past year)"
    )
    public static let dateIntervalAll = NSLocalizedString(
      "wallet.dateIntervalAll",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "All",
      comment: "Describes what range of data to show on the graph (all data available)"
    )
    public static let swapCryptoFromTitle = NSLocalizedString(
      "wallet.swapCryptoFromTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "From",
      comment: "A title above the cryptocurrency token/asset you are swapping from. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let swapCryptoToTitle = NSLocalizedString(
      "wallet.swapCryptoToTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "To",
      comment: "A title above the cryptocurrency token/asset you are swapping to. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let swapCryptoAmountTitle = NSLocalizedString(
      "wallet.swapCryptoAmountTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter amount of %@ to swap",
      comment: "A title above the amount of asset you want to swap. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapCryptoAmountReceivingTitle = NSLocalizedString(
      "wallet.swapCryptoAmountReceivingTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Amount receiving in %@ (estimated)",
      comment: "A title above the amount of asset you will receive from the swap. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapOrderTypeLabel = NSLocalizedString(
      "wallet.swapOrderTypeLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Order Type",
      comment: "The type of order you want to place. Options are: 'Market' and 'Limit'"
    )
    public static let swapLimitOrderType = NSLocalizedString(
      "wallet.swapLimitOrderType",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Limit",
      comment: "The 'Limit' order type. Limit orders only execute when the price requirements are met"
    )
    public static let swapMarketOrderType = NSLocalizedString(
      "wallet.swapMarketOrderType",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Market",
      comment: "The 'Market' order type. Market orders execute immediately based on the price at the time of the order."
    )
    public static let today = NSLocalizedString(
      "wallet.today",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Today",
      comment: "A label appended after a certain dollar or percent change. Example: 'Up 1.4% Today'"
    )
    public static let selectAccountTitle = NSLocalizedString(
      "wallet.selectAccountTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Select Account",
      comment: "The title of the account selection screen. Will show above a list of accounts the user may pick from"
    )
    public static let assetDetailSubtitle = NSLocalizedString(
      "wallet.assetDetailSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "%@ Price (%@)",
      comment: "A subtitle on the asset details screen that uses the name and symbol. Example: Basic Attention Token Price (BAT)"
    )
    public static let biometricsSetupTitle = NSLocalizedString(
      "wallet.biometricsSetupTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unlock Brave Wallet with your Face ID, Touch ID, or password.",
      comment: "The title shown when a user is asked if they would like to setup biometric unlock"
    )
    public static let biometricsSetupEnableButtonTitle = NSLocalizedString(
      "wallet.biometricsSetupEnableButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enable",
      comment: "The button title that enables the biometric unlock feature"
    )
    public static let copyAddressButtonTitle = NSLocalizedString(
      "wallet.copyAddressButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Copy Address",
      comment: "The button title that appears when long-pressing a wallet address that will copy said address to the users clipboard"
    )
    public static let autoLockTitle = NSLocalizedString(
      "wallet.autoLockTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Automatically Lock After",
      comment: "The title that appears before an auto-lock interval. Example: Automatically lock after 5 minutes"
    )
    public static let autoLockFooter = NSLocalizedString(
      "wallet.autoLockFooter",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "The number of minutes to wait until the Brave Wallet is automatically locked",
      comment: "The footer beneath the auto-lock title and interval duration"
    )
    public static let enterAmount = NSLocalizedString(
      "wallet.enterAmount",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter amount",
      comment: "The header title for the textField users will input the dollar value of the crypto they want to buy"
    )
    public static let amountInCurrency = NSLocalizedString(
      "wallet.amountInCurrency",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Amount in %@",
      comment: "A placeholder on a text field to describe an amount of some currency. '%@' will be replaced with a currency code such as 'USD' or 'BAT'"
    )
    public static let buyButtonTitle = NSLocalizedString(
      "wallet.buyButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Continue to Wyre",
      comment: "The title of the button for users to click when they are ready to buy using Wyre payment"
    )
    public static let buyTestTitle = NSLocalizedString(
      "wallet.buyTestTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Test Faucet",
      comment: "The title below account picker when user has selected a test network"
    )
    public static let buyTestDescription = NSLocalizedString(
      "wallet.buyTestDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Get Ether from a faucet for %@",
      comment: "The description of where user will go to once a test network has been picked in buy screen. '%@' will be replaced with a network such as 'Rinkeby' or 'Ropsten'"
    )
    public static let buyTestButtonTitle = NSLocalizedString(
      "wallet.buyTestButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Get Ether",
      comment: "The title of the button for users to get ether if the test network has been chosen"
    )
    public static let sendCryptoFromTitle = NSLocalizedString(
      "wallet.sendCryptoFromTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "From",
      comment: "A title above the cryptocurrency token/asset you are sending from. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let sendCryptoAmountTitle = NSLocalizedString(
      "wallet.sendCryptoAmountTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter amount of %@ to send",
      comment: "A title above the amount of asset you want to send. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let sendCryptoToTitle = NSLocalizedString(
      "wallet.sendCryptoToTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "To",
      comment: "A title above the address you want to send to. For example this would appear over a cell that has the 'OxFCdf***DDee' with a clipboard icon and a qr-code icon on the right hand side"
    )
    public static let sendCryptoAddressPlaceholder = NSLocalizedString(
      "wallet.sendCryptoAddressPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter address or url",
      comment: "A placeholder of the address text field."
    )
    public static let scanQRCodeAccessibilityLabel = NSLocalizedString(
      "wallet.scanQRCodeAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Scan QR code",
      comment: "A description for a QR code icon which brings up the camera to read ETH addresses encoded as QR codes"
    )
    public static let sendCryptoSendButtonTitle = NSLocalizedString(
      "wallet.sendCryptoSendButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Send",
      comment: "The title of the button for users to click when they want to send the sending-transaction"
    )
    public static let sendCryptoSendError = NSLocalizedString(
      "wallet.sendCryptoSendError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "We currently cannot proceed with your transaction",
      comment: "The error message will appear when there is any error occurs during unpproved transaction"
    )
    public static let swapCryptoUnsupportNetworkTitle = NSLocalizedString(
      "wallet.swapCryptoUnsupportNetworkTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unsupported chain",
      comment: "The title below account picker when user has selected a test network to swap cryptos"
    )
    public static let swapCryptoUnsupportNetworkDescription = NSLocalizedString(
      "wallet.swapCryptoUnsupportNetworkBody",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Brave doesn't currently support swap on the %@. Please switch to a supported chain.",
      comment: "The description of where user will see once a test network has been picked in swap screen. '%@' will be replaced with a network such as 'Rinkeby Test Network' or 'Ropsten Test Network'"
    )
    public static let swapCryptoSlippageTitle = NSLocalizedString(
      "wallet.swapCryptoSlippageTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Slippage Tolerance",
      comment: "The title for Slippage tolerance field. User will choose the tolerance for slippage as a percentage value"
    )
    public static let swapCryptoSwapButtonTitle = NSLocalizedString(
      "wallet.swapCryptoSwapButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Swap",
      comment: "The title of the button for users to click when they want to swap between two cryptos"
    )
    public static let swapCryptoMarketPriceTitle = NSLocalizedString(
      "wallet.swapCryptoMarketPriceTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Market Price in %@",
      comment: "The title of the field for display the market price of the crypto that user chooses to swap from. The title lives above the price label. '%@' will be replaced with the symbol of the crypto that users choose to swap from as 'ETH' or 'BAT'"
    )
    public static let refreshMarketPriceLabel = NSLocalizedString(
      "wallet.refreshMarketPriceLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Refresh market price",
      comment: "A description for a refresh icon that when pressed receives a new snap quote for the currently swap assets"
    )
    public static let swapSelectedTokens = NSLocalizedString(
      "wallet.swapSelectedTokens",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Swap selected tokens",
      comment: "An accessibility message for the swap button below from amount shortcut grids for users to swap the two selected tokens."
    )
    public static let transactionCount = NSLocalizedString(
      "wallet.transactionCount",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "%lld of %lld",
      comment: "Displays the number of transactions and the current transaction that you are viewing when confirming or rejecting multiple transactions. Each '%lld' will be replaced by a number, for example: '1 of 4'"
    )
    public static let next = NSLocalizedString(
      "wallet.next",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Next",
      comment: "1. A button title next indicating the user to go to the next transaction. Will sit next to a label such as \"1 of 4\" where tapping next would move them to the second transaction. 2. Title of the button for users to click to the next step during the process of dapp permission requests."
    )
    public static let transactionFromToAccessibilityLabel = NSLocalizedString(
      "wallet.transactionFromToAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "From: %@. To: %@",
      comment: "A VoiceOver label that will be read out when a user focuses a transactions \"from address\" and \"to address\" labels. \"%@\" will be replaced with either an account name or a truncated ethereum address such as \"Account 1\" or \"0x1234***3003\""
    )
    public static let confirmationViewModeTransaction = NSLocalizedString(
      "wallet.confirmationViewModeTransaction",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transaction",
      comment: "One of the picker options while confirming a transaction. When selected it displays a summary of the transaction such as value, gas fee, and totals"
    )
    public static let confirmationViewModeDetails = NSLocalizedString(
      "wallet.confirmationViewModeDetails",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Details",
      comment: "One of the picker options while confirming a transaction. When selected it displays a transactions function details such as underlying data"
    )
    public static let confirmationViewEditPermissions = NSLocalizedString(
      "wallet.confirmationViewEditPermissions",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Edit permissions",
      comment: "The title shown on the button to edit permissions / the allowance for an ERC 20 Approve transaction while confirming a transaction."
    )
    public static let confirmationViewAllowSpendTitle = NSLocalizedString(
      "wallet.confirmationViewAllowSpendTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Allow this app to spend your %@?",
      comment: "The title shown on transaction confirmation for an ERC 20 Approve transaction where the '%@' will be the name of the symbol being approved. For example: \"Allow this app to spend your DAI?\""
    )
    public static let confirmationViewAllowSpendSubtitle = NSLocalizedString(
      "wallet.confirmationViewAllowSpendSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "By granting this permission, you are allowing this app to withdraw your %@ and automate transactions for you.",
      comment: "The subtitle shown on transaction confirmation for an ERC 20 Approve transaction where the '%@' will be the name of the symbol being approved. For example: \"By granting this permission, you are allowing this app to withdraw your DAI and automate transactions for you.\""
    )
    public static let confirmationViewUnlimitedWarning = NSLocalizedString(
      "wallet.confirmationViewUnlimitedWarning",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unlimited approval requested",
      comment: "The warning shown on transaction confirmation for an ERC 20 Approve transaction when the proposed allowance is unlimited."
    )
    public static let confirmationViewCurrentAllowance = NSLocalizedString(
      "wallet.confirmationViewCurrentAllowance",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Current Allowance",
      comment: "The label shown beside the current allowance for a ERC20 approve transaction where the current allowance is a number followed by the symbol name. For example, \"Current Allowance  100 DAI\"."
    )
    public static let gasFee = NSLocalizedString(
      "wallet.gasFee",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Gas Fee",
      comment: "A title displayed beside a number describing the cost of the transaction in ETH which is called Gas"
    )
    public static let editGasFeeButtonTitle = NSLocalizedString(
      "wallet.editGasFeeButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Edit",
      comment: "A button title displayed under a Gas Fee title that allows the user to adjust the gas fee/transactions priority"
    )
    public static let total = NSLocalizedString(
      "wallet.total",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Total",
      comment: "A title displayed beside a number describing the total amount of gas and ETH that will be transferred"
    )
    public static let amountAndGas = NSLocalizedString(
      "wallet.amountAndGas",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Amount + Gas",
      comment: "A title displayed above two numbers (the amount and gas) showing the user the breakdown of the amount transferred and gas fee. The \"+\" is a literal plus as the label below will show such as \"0.004 ETH + 0.00064 ETH\""
    )
    public static let inputDataPlaceholder = NSLocalizedString(
      "wallet.inputDataPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No data.",
      comment: "A label shown inside of a box when there is no input data for a given transaction"
    )
    public static let rejectAllTransactions = NSLocalizedString(
      "wallet.rejectAllTransactions",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Reject %d Transactions",
      comment: "A button title that allows the user to reject all unapproved transactions at once. %d will be replaced with a number, example: Reject 4 Transactions"
    )
    public static let confirmTransactionTitle = NSLocalizedString(
      "wallet.confirmTransactionTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Confirm Transaction",
      comment: "The title of the transaction confirmation panel UI."
    )
    public static let confirmTransactionsTitle = NSLocalizedString(
      "wallet.confirmTransactionsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Confirm Transactions",
      comment: "The title of the transaction confirmation panel UI when there are multiple transactions to confirm"
    )
    public static let confirm = NSLocalizedString(
      "wallet.confirm",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Confirm",
      comment: "1. A button title to confirm a transaction. It is shown below details about a given transaction. 2. A button title to confirm to grant permission of a dapp connect request. It is displayed at the bottom of last step to grant permission in new site connection screen."
    )
    public static let rejectTransactionButtonTitle = NSLocalizedString(
      "wallet.rejectTransactionButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Reject",
      comment: "A button title to reject a transaction. It is shown below details about a given transaction"
    )
    public static let insufficientFunds = NSLocalizedString(
      "wallet.insufficientFunds",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Insufficient funds",
      comment: "An error message displayed when the user does not have enough funds to make or confirm a transaction"
    )
    public static let gasFeeDisclaimer = NSLocalizedString(
      "wallet.gasFeeDisclaimer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "While not a guarantee, miners will likely prioritize your transaction earlier if you pay a higher fee.",
      comment: "A disclaimer shown above the UI to select a gas fee"
    )
    public static let gasFeePredefinedLimitLow = NSLocalizedString(
      "wallet.gasFeePredefinedLimitLow",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Low",
      comment: "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom"
    )
    public static let gasFeePredefinedLimitOptimal = NSLocalizedString(
      "wallet.gasFeePredefinedLimitOptimal",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Optimal",
      comment: "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom"
    )
    public static let gasFeePredefinedLimitHigh = NSLocalizedString(
      "wallet.gasFeePredefinedLimitHigh",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "High",
      comment: "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom"
    )
    public static let gasFeeCustomOption = NSLocalizedString(
      "wallet.gasFeeCustomOption",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Custom",
      comment: "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom. This option allows the user to specify gas fee details themselves"
    )
    public static let gasCurrentBaseFee = NSLocalizedString(
      "wallet.gasCurrentBaseFee",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Current base fee",
      comment: "The base cost of the gas fee before adjustments by the user. It will be shown next to a Gwei amount"
    )
    public static let gasAmountLimit = NSLocalizedString(
      "wallet.gasAmountLimit",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Gas amount limit",
      comment: "A title above a text field for inputting the gas amount limit"
    )
    public static let perGasTipLimit = NSLocalizedString(
      "wallet.perGasTipLimit",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Per-gas tip limit (Gwei)",
      comment: "A title above a text field for inputting the per-gas tip limit in Gwei"
    )
    public static let perGasPriceLimit = NSLocalizedString(
      "wallet.perGasPriceLimit",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Per-gas price limit (Gwei)",
      comment: "A title above a text field for inputting the per-gas price limit in Gwei"
    )
    public static let maximumGasFee = NSLocalizedString(
      "wallet.maximumGasFee",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Maximum fee",
      comment: "The highest the user will pay in a gas fee based on the entered gas fee details or predefined option. It is displayed above the amount"
    )
    public static let saveButtonTitle = NSLocalizedString(
      "wallet.saveButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Save",
      comment: "A button title for saving the users selected gas fee options. Or to save a custom nonce value. Or to save a custom network"
    )
    public static let maxPriorityFeeTitle = NSLocalizedString(
      "wallet.maxPriorityFeeTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Max Priority Fee",
      comment: "The title of the edit gas fee screen for EIP-1559 transactions"
    )
    public static let insufficientBalance = NSLocalizedString(
      "wallet.insufficientBalance",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Insufficient balance",
      comment: "An error message when there is no insufficient balance for swapping. It will be displayed as the title of the disabled swap button at the bottom in the Swap Screen."
    )
    public static let insufficientFundsForGas = NSLocalizedString(
      "wallet.insufficientFundsForGas",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Insufficient funds for gas",
      comment: "An error message when there is no insufficient funds for gas fee. It will be displayed as the title of the disabled swap button at the bottom in the Swap Screen."
    )
    public static let activateToken = NSLocalizedString(
      "wallet.activateToken",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Activate Token %@",
      comment: "The title of the button at the bottom of Swap Screen, when the sell token is erc20 and it has not been activated its allowance. %@ will be replaced with the sell token's symbol such as 'DAI' or 'USDC'"
    )
    public static let insufficientLiquidity = NSLocalizedString(
      "wallet.insufficientLiquidity",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Insufficient liquidity",
      comment: "An error message displayed when the user doesn't have enough liquidity to proceed with a transaction."
    )
    public static let unknownError = NSLocalizedString(
      "wallet.unknownError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unknown error",
      comment: "An error message displayed when an unspecified problem occurs."
    )
    public static let transactionSummaryFee = NSLocalizedString(
      "wallet.transactionSummaryFee",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Fee: %@ %@ (%@)",
      comment: "A transactions gas fee. The first '%@' becomes the fee amount, the second '%@' becomes the symbol for the fee's currency and the last '%@' becomes the fiat amount. For example: \"Fee: 0.0054 ETH ($22.44)\""
    )
    public static let transactionApproveSymbolTitle = NSLocalizedString(
      "wallet.transactionApproveSymbolTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Approved %@ %@",
      comment: "The title shown for ERC20 approvals. The first '%@' becomes the  amount, the second '%@' becomes the symbol for the cryptocurrency. For example: \"Approved 150.0 BAT\""
    )
    public static let transactionUnknownApprovalTitle = NSLocalizedString(
      "wallet.transactionUnknownApprovalTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Approved",
      comment: "The title shown for ERC20 approvals when the user doesn't have the visible asset added"
    )
    public static let transactionSwapTitle = NSLocalizedString(
      "wallet.transactionSwapTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Swapped %@ %@ (%@)",
      comment: "A title shown for a swap transaction. The first '%@' becomes the  amount, the second '%@' becomes the symbol for the cryptocurrency and the last '%@' becomes the fiat amount. For example: \"Swapped 0.0054 ETH ($22.44)\""
    )
    public static let transactionSendTitle = NSLocalizedString(
      "wallet.transactionSendTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Sent %@ %@ (%@)",
      comment: "A title shown for a send transaction. The first '%@' becomes the  amount, the second '%@' becomes the symbol for the cryptocurrency and the last '%@' becomes the fiat amount. For example: \"Sent 0.0054 ETH ($22.44)\""
    )
    public static let transactionUnknownSendTitle = NSLocalizedString(
      "wallet.transactionUnknownSendTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Sent %@",
      comment: "A title shown for a erc 20 transfer, or erc 721 transaction. The first '%@' becomes the symbol for the cryptocurrency For example: \"Sent ETH\""
    )
    public static let viewOnBlockExplorer = NSLocalizedString(
      "wallet.viewOnBlockExplorer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "View on block explorer",
      comment: "A button title to view a given transaction on the block explorer for the current network/chain."
    )
    public static let transactionStatusConfirmed = NSLocalizedString(
      "wallet.transactionStatusConfirmed",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Confirmed",
      comment: "A status that explains that the transaction has been completed/confirmed"
    )
    public static let transactionStatusApproved = NSLocalizedString(
      "wallet.transactionStatusApproved",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Approved",
      comment: "A status that explains that the transaction has been approved by the user"
    )
    public static let transactionStatusRejected = NSLocalizedString(
      "wallet.transactionStatusRejected",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Rejected",
      comment: "A status that explains that the transaction has been rejected by the user"
    )
    public static let transactionStatusUnapproved = NSLocalizedString(
      "wallet.transactionStatusUnapproved",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unapproved",
      comment: "A status that explains that a transaction has not yet been approved"
    )
    public static let transactionStatusSubmitted = NSLocalizedString(
      "wallet.transactionStatusSubmitted",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Submitted",
      comment: "A status that explains that the transaction has been submitted to the blockchain"
    )
    public static let transactionStatusDropped = NSLocalizedString(
      "wallet.transactionStatusDropped",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Dropped",
      comment: "A status that explains that the transaction has been dropped due to some error"
    )
    public static let transactionStatusError = NSLocalizedString(
      "wallet.transactionStatusError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Error",
      comment: "A status that explains that the transaction failed due to some error"
    )
    public static let transactionStatusUnknown = NSLocalizedString(
      "wallet.transactionStatusUnknown",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unknown",
      comment: "A transaction status that the app currently does not support displaying"
    )
    public static let customTokenTitle = NSLocalizedString(
      "wallet.customTokenTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Custom",
      comment: "The title displayed on the add custom token screen"
    )
    public static let tokenName = NSLocalizedString(
      "wallet.tokenName",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Token name",
      comment: "A title that will be displayed on top of the text field for users to input the custom token name"
    )
    public static let enterTokenName = NSLocalizedString(
      "wallet.enterTokenName",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter token name",
      comment: "A placeholder for the text field that users will input the custom token name"
    )
    public static let tokenContractAddress = NSLocalizedString(
      "wallet.tokenContractAddress",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Token contract address",
      comment: "A title that will be displayed on top of the text field for users to input the custom token contract address"
    )
    public static let enterContractAddress = NSLocalizedString(
      "wallet.enterContractAddress",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter contract address",
      comment: "A placeholder for the text field that users will input the custom token contract address"
    )
    public static let tokenSymbol = NSLocalizedString(
      "wallet.tokenSymbol",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Token symbol",
      comment: "A title that will be displayed on top of the text field for users to input the custom token symbol"
    )
    public static let enterTokenSymbol = NSLocalizedString(
      "wallet.enterTokenSymbol",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter token symbol",
      comment: "A placeholder for the text field that users will input the custom token symbol"
    )
    public static let decimalsPrecision = NSLocalizedString(
      "wallet.decimalsPrecision",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Decimals of precision",
      comment: "A title that will be displayed on top of the text field for users to input the custom token's decimals of precision"
    )
    public static let addCustomTokenErrorTitle = NSLocalizedString(
      "wallet.addCustomTokenErrorTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Cannot add custom token",
      comment: "The title of the error pop up when there is an error occurs during the process of adding a custom token."
    )
    public static let addCustomTokenErrorMessage = NSLocalizedString(
      "wallet.addCustomTokenErrorMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Failed to add custom token, please try again.",
      comment: "The message of the error pop up when there is an error occurs during the process of adding a custom token."
    )
    public static let removeCustomTokenErrorTitle = NSLocalizedString(
      "wallet.removeCustomTokenErrorTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Cannot remove custom token",
      comment: "The title of the error pop up when there is an error occurs during the process of removing a custom token."
    )
    public static let removeCustomTokenErrorMessage = NSLocalizedString(
      "wallet.removeCustomTokenErrorMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Please verify this is a new custom asset, check your internet connection, and try again.",
      comment: "The message in the error pop up when there is an error occurs during the process of removing a custom token."
    )
    public static let addCustomAsset = NSLocalizedString(
      "wallet.addCustomAsset",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add custom asset",
      comment: "The title of the button that is located in the same area of the assets list header but on the right side. Users will click it and go to add custom asset screen."
    )
    public static let delete = NSLocalizedString(
      "wallet.delete",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Delete",
      comment: "The title of the option inside the context menu for custom asset row in edit user asset screen. Or the title of the option inside the context menu for custom network row which is not being currently selected in networks list screen."
    )
    public static let transactionTypeApprove = NSLocalizedString(
      "wallet.transactionTypeApprove",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Approve",
      comment: "Explains that this transaction is an ERC20 approval transaction and is displayed among other transaction info"
    )
    public static let perGasPriceTitle = NSLocalizedString(
      "wallet.perGasPriceTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Per-gas price (Gwei)",
      comment: "A title above a text field for inputting the per-gas price limit in Gwei"
    )
    public static let editGasTitle = NSLocalizedString(
      "wallet.editGasTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Edit Gas",
      comment: "A title of the edit gas screen for standard transactions"
    )
    public static let lock = NSLocalizedString(
      "wallet.lock",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Lock",
      comment: "The title of the lock option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let settings = NSLocalizedString(
      "wallet.settings",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Settings",
      comment: "The title of the settings option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let swapDexAggrigatorNote = NSLocalizedString(
      "wallet.swapDexAggrigatorNote",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Brave uses 0x as a DEX aggregator.",
      comment: "A disclaimer note shown on the Swap screen. '0x' is a company name. 'DEX aggregator' is a type of blockchain-based service (decentralized exchange)"
    )
    public static let swapDexAggrigatorDisclaimer = NSLocalizedString(
      "wallet.swapDexAggrigatorDisclaimer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "0x will process the Ethereum address and IP address to fulfill a transaction (including getting quotes). 0x will ONLY use this data for the purposes of processing transactions.",
      comment: "A longer disclaimer about the DEX aggrigator used by Brave for swap transactions. '0x' is a company name. 'DEX aggregator' is a type of blockchain-based service (decentralized exchange). 'ONLY' is emphasized to show importance of 0x's data usage."
    )
    public static let chartAxisDateLabel = NSLocalizedString(
      "wallet.chartAxisDateLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Date",
      comment: "The x-axis label on an asset or portfolio chart describing that x-axis values are defined by the date of the price"
    )
    public static let chartAxisPriceLabel = NSLocalizedString(
      "wallet.chartAxisPriceLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Price",
      comment: "The y-axis label on an asset or portfolio chart describing that y-axis values are based on the price at a given date"
    )
    public static let coinGeckoDisclaimer = NSLocalizedString(
      "wallet.coinGeckoDisclaimer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Price data powered by CoinGecko",
      comment: "A disclaimer that appears at the bottom of an asset detail screen which shows prices and price history. CoinGecko is a third-party product."
    )
    public static let braveSwapFeeDisclaimer = NSLocalizedString(
      "wallet.braveSwapFeeDisclaimer",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Quote includes a %@ Brave fee.",
      comment: "A disclaimer that appears at the bottom of an swap screen which discloses the fixed Brave fee included in the swap quotes. '%@' will be replaced by a percentage. For example: 'Quote includes a 0.875% Brave fee'"
    )
    public static let screenshotDetectedTitle = NSLocalizedString(
      "wallet.screenshotDetectedTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Screenshot detected",
      comment: "A title of an alert when the user takes a screenshot of their device"
    )
    public static let recoveryPhraseScreenshotDetectedMessage = NSLocalizedString(
      "wallet.recoveryPhraseScreenshotDetectedMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Warning: A screenshot of your recovery phrase may get backed up to a cloud file service, and be readable by any application with photos access. Brave recommends that you not save this screenshot, and delete it as soon as possible.",
      comment: "The message displayed when the user takes a screenshot of their recovery phrase"
    )
    public static let privateKeyScreenshotDetectedMessage = NSLocalizedString(
      "wallet.privateKeyScreenshotDetectedMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Warning: A screenshot of your private key may get backed up to a cloud file service, and be readable by any application with photos access. Brave recommends that you not save this screenshot, and delete it as soon as possible.",
      comment: "The message displayed when the user takes a screenshot of their private key"
    )
    public static let sendWarningAddressIsOwn = NSLocalizedString(
      "wallet.sendWarningAddressIsOwn",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "The receiving address is your own address",
      comment: "A warning that appears below the send crypto address text field, when the input `To` address is the same as the current selected account's address."
    )
    public static let sendWarningAddressIsContract = NSLocalizedString(
      "wallet.sendWarningAddressIsContract",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "The receiving address is a token's contract address",
      comment: "A warning that appears below the send crypto address text field, when the input `To` address is a token contract address."
    )
    public static let sendWarningAddressNotValid = NSLocalizedString(
      "wallet.sendWarningAddressNotValid",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Not a valid ETH address",
      comment: "A warning that appears below the send crypto address text field, when the input `To` address is not a valid ETH address."
    )
    public static let sendWarningAddressMissingChecksumInfo = NSLocalizedString(
      "wallet.sendWarningAddressMissingChecksumInfo",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This address cannot be verified (missing checksum). Proceed?",
      comment: "A warning that appears below the send crypto address text field, when the input `To` address is missing checksum information."
    )
    public static let sendWarningAddressInvalidChecksum = NSLocalizedString(
      "wallet.sendWarningAddressInvalidChecksum",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Address did not pass verification (invalid checksum). Please try again, replacing lowercase letters with uppercase.",
      comment: "A warning that appears below the send crypto address text field, when the input `To` address has invalid checksum."
    )
    public static let customNetworkChainIdTitle = NSLocalizedString(
      "wallet.customNetworkChainIdTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "The ID of the chain",
      comment: "The title above the input field for users to input network chain ID in Custom Network Details Screen."
    )
    public static let customNetworkChainIdPlaceholder = NSLocalizedString(
      "wallet.customNetworkChainIdPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter chain ID",
      comment: "The placeholder for the input field for users to input network chain ID in Custom Network Details Screen."
    )
    public static let customNetworkChainIdErrMsg = NSLocalizedString(
      "wallet.customNetworkChainIdErrMsg",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Invalid format—chain ID must be a positive number.",
      comment: "The error message for the input field when users input an invalid value for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkChainNameTitle = NSLocalizedString(
      "wallet.customNetworkChainNameTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "The name of the chain",
      comment: "The title above the input field for users to input network chain name in Custom Network Details Screen."
    )
    public static let customNetworkChainNamePlaceholder = NSLocalizedString(
      "wallet.customNetworkChainNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter chain name",
      comment: "The placeholder for the input field for users to input network chain name in Custom Network Details Screen."
    )
    public static let customNetworkEmptyErrMsg = NSLocalizedString(
      "wallet.customNetworkEmptyErrMsg",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This field cannot be blank.",
      comment: "The error message for the input fields cannot be empty in Custom Network Details Screen."
    )
    public static let customNetworkSymbolNameTitle = NSLocalizedString(
      "wallet.customNetworkSymbolNameTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Chain's currency name",
      comment: "The title above the input field for users to input network currency symbol name in Custom Network Details Screen."
    )
    public static let customNetworkSymbolNamePlaceholder = NSLocalizedString(
      "wallet.customNetworkSymbolNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter currency name",
      comment: "The placeholder for the input field for users to input network currency symbol name in Custom Network Details Screen."
    )
    public static let customNetworkSymbolTitle = NSLocalizedString(
      "wallet.customNetworkSymbolTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Chain's currency symbol",
      comment: "The title above the input field for users to input network currency symbol in Custom Network Details Screen."
    )
    public static let customNetworkSymbolPlaceholder = NSLocalizedString(
      "wallet.customNetworkSymbolPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter currency symbol",
      comment: "The placeholder for the input field for users to input network currency symbol in Custom Network Details Screen."
    )
    public static let customNetworkCurrencyDecimalTitle = NSLocalizedString(
      "wallet.customNetworkCurrencyDecimalTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Chain's currency decimals",
      comment: "The title above the input field for users to input network currency decimal in Custom Network Details Screen."
    )
    public static let customNetworkCurrencyDecimalPlaceholder = NSLocalizedString(
      "wallet.customNetworkCurrencyDecimalPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter currency decimals",
      comment: "The placeholder for the input field for users to input network currency decimal in Custom Network Details Screen."
    )
    public static let customNetworkCurrencyDecimalErrMsg = NSLocalizedString(
      "wallet.customNetworkCurrencyDecimalErrMsg",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Invalid format—currency decimals must be a positive number.",
      comment: "The error message for the input field when users input an invalid value for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkRpcUrlsTitle = NSLocalizedString(
      "wallet.customNetworkRpcUrlsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "RPC URLs",
      comment: "The title above the input fields for users to input network rpc urls in Custom Network Details Screen."
    )
    public static let customNetworkUrlsPlaceholder = NSLocalizedString(
      "wallet.customNetworkUrlsPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter URL(s)",
      comment: "The placeholder for the input field for users to input network rpc url in Custom Network Details Screen."
    )
    public static let customNetworkInvalidAddressErrMsg = NSLocalizedString(
      "wallet.customNetworkInvalidAddressErrMsg",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Invalid address.",
      comment: "The error message for the input field when users input an invalid address for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkIconUrlsTitle = NSLocalizedString(
      "wallet.customNetworkIconUrlsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Icon URLs",
      comment: "The title above the input fields for users to input network icon urls in Custom Network Details Screen."
    )
    public static let customNetworkBlockExplorerUrlsTitle = NSLocalizedString(
      "wallet.customNetworkBlockExplorerUrlsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Block explorer URLs",
      comment: "The title above the input fields for users to input network block explorer urls in Custom Network Details Screen."
    )
    public static let editfCustomNetworkTitle = NSLocalizedString(
      "wallet.editfCustomNetworkTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Edit Network",
      comment: "The title of form for users to edit an existed custom network."
    )
    public static let customNetworkDetailsTitle = NSLocalizedString(
      "wallet.customNetworkDetailsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add New Network",
      comment: "The title for Custom Network Details Screen for user to add or edit custom network."
    )
    public static let viewNetworkDetailsTitle = NSLocalizedString(
      "wallet.viewNetworkDetailsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Network Details",
      comment: "The title for View Network Details Screen shown when a user views the details of a network they are requested to add or switch to by a dapps website."
    )
    public static let settingsNetworkButtonTitle = NSLocalizedString(
      "wallet.settingsNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Networks",
      comment: "The title of a button that will go the list of networks."
    )
    public static let networkFooter = NSLocalizedString(
      "wallet.networkFooter",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Wallet networks customization",
      comment: "The footer beneath the networks title in settings screen."
    )
    public static let networkIdDuplicationErrMsg = NSLocalizedString(
      "wallet.networkIdDuplicationErrMsg",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Chain ID already exists",
      comment: "An error message will pop up when the user tries to add a custom network that its id already exists."
    )
    public static let failedToAddCustomNetworkErrorTitle = NSLocalizedString(
      "wallet.failedToAddCustomNetworkErrorTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Failed to add this network.",
      comment: "The title of an alert when the custom network the user attempted to add fails for some reason"
    )
    public static let failedToRemoveCustomNetworkErrorMessage = NSLocalizedString(
      "wallet.failedToRemoveCustomNetworkErrorMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Failed to remove network.\nPlease try again.",
      comment: "The message of an alert when the user attempted to remove custom network and it fails for some reason"
    )
    public static let customNetworksTitle = NSLocalizedString(
      "wallet.customNetworksTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Custom Networks",
      comment: "The title of custom network list screen"
    )
    public static let addCustomNetworkBarItemTitle = NSLocalizedString(
      "wallet.addCustomNetworkBarItemTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add Network",
      comment: "The title of bar item for users to add custom network screen"
    )
    public static let addCustomNetworkDropdownButtonTitle = NSLocalizedString(
      "wallet.addCustomNetworkDropdownButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add Network…",
      comment: "The title of last option in the network selection dropdown menu. A short-cut for user to add new custom network."
    )
    public static let noNetworks = NSLocalizedString(
      "wallet.noNetworks",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "No Networks Added",
      comment: "The empty state displayed when the user has not yet added any custom networks."
    )
    public static let transactionBacklogTitle = NSLocalizedString(
      "wallet.transactionBacklogTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transaction backlog",
      comment: "Title of confirmation prompt when there's a backlog of wallet transactions"
    )
    public static let transactionBacklogBody = NSLocalizedString(
      "wallet.transactionBacklogBody",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "You're attempting to start a new transaction while you have previously submitted transactions that have not been confirmed. This will block any new ones from being submitted.",
      comment: "Text body of confirmation prompt when there's a backlog of wallet transactions"
    )
    public static let transactionBacklogAcknowledgement = NSLocalizedString(
      "wallet.transactionBacklogAcknowledgement",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Clear & replace the incomplete transaction(s)",
      comment: "Text of toggle for a confirmation prompt when there's a backlog of wallet transactions"
    )
    public static let learnMoreButton = NSLocalizedString(
      "wallet.learnMoreButton",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Learn More",
      comment: "Button to learn more about incomplete/pending wallet transactions. Or learn more about adding/switching networks in a dapp request view"
    )
    public static let transactionBacklogAfterReplacement = NSLocalizedString(
      "wallet.transactionBacklogAfterReplacement",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "For now, no additional action is required. Just wait for the unconfirmed transactions to clear.",
      comment: "Additional information, when users have previously clear and replace the incomplete transaction(s)"
    )
    public static let settingsResetTransactionTitle = NSLocalizedString(
      "wallet.settingsResetTransactionTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Clear Transaction & Nonce Info",
      comment: "The title of a button that will reset transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let settingsResetTransactionFooter = NSLocalizedString(
      "wallet.settingsResetTransactionFooter",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Clearing transactions may be useful for developers or when clearing state on a local server",
      comment: "The footer message below the button to reset transaction"
    )
    public static let settingsResetTransactionAlertTitle = NSLocalizedString(
      "wallet.settingsResetTransactionAlertTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Are you sure you want to reset transacton and nonce information?",
      comment: "The title the confirmation dialog when resetting transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let settingsResetTransactionAlertMessage = NSLocalizedString(
      "wallet.settingsResetTransactionAlertMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This option is mostly used by developers running a local test server",
      comment: "The message the confirmation dialog when resetting transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let settingsResetTransactionAlertButtonTitle = NSLocalizedString(
      "wallet.settingsResetTransactionAlertButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Reset",
      comment: "The title of a button that will reset transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let advancedSettingsTransaction = NSLocalizedString(
      "wallet.advancedSettingsTransaction",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Advanced settings",
      comment: "The title of the button that is displayed under the total gas fee in the confirmation transaction screen. Users can click it to go to advanced settings screen."
    )
    public static let editNonceHeader = NSLocalizedString(
      "wallet.editNonceHeader",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Nonce",
      comment: "The header title for edit nonce section in advanced settings screen."
    )
    public static let editNonceFooter = NSLocalizedString(
      "wallet.editNonceFooter",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transaction may not be propagated to the network if custom nonce value is non-sequential or has other errors",
      comment: "The footer title for edit nonce section in advanced settings screen."
    )
    public static let editNoncePlaceholder = NSLocalizedString(
      "wallet.editNoncePlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter custom nonce value",
      comment: "The placeholder for custom nonce textfield."
    )
    public static let editTransactionError = NSLocalizedString(
      "wallet.editTransactionError",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Please check your inputs and try again.",
      comment: "The error alert body when something is wrong when users try to edit transaction gas fee, priority fee, nonce value or allowance value."
    )
    public static let editTransactionErrorCTA = NSLocalizedString(
      "wallet.editTransactionErrorCTA",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Go back",
      comment: "The transaction edit error alert button which will dismiss the alert."
    )
    public static let transactionDetailsTitle = NSLocalizedString(
      "wallet.transactionDetailsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transaction Details",
      comment: "The title for the view displaying the details of a cryptocurrency transaction."
    )
    public static let transactionDetailsTxFeeTitle = NSLocalizedString(
      "wallet.transactionDetailsTxFeeTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transaction fee",
      comment: "The label for the fees involved in a cryptocurrency transaction. Appears next to the value transferred and the currency amount."
    )
    public static let transactionDetailsMarketPriceTitle = NSLocalizedString(
      "wallet.transactionDetailsMarketPriceTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Market price",
      comment: "The label for the market price of the asset used in a cryptocurrency transaction. Appears next to the formatted currency such as $1523.50"
    )
    public static let transactionDetailsDateTitle = NSLocalizedString(
      "wallet.transactionDetailsDateTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Date",
      comment: "The label for displaying the date a transaction occurred. Appears next to the formatted date such as '3:00PM - Jan 1 2022'"
    )
    public static let transactionDetailsNetworkTitle = NSLocalizedString(
      "wallet.transactionDetailsNetworkTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Network",
      comment: "The label for the network a transaction occurred on. Appears next to 'Ethereum Mainnet', 'Rinkeby Test Network', 'Ropsten Test Network', etc. "
    )
    public static let transactionDetailsTxHashTitle = NSLocalizedString(
      "wallet.transactionDetailsTransactionHashTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Transaction hash",
      comment: "The label for the transaction hash (the identifier) of a cryptocurrency transaction. Appears next to a button that opens a URL for the transaction."
    )
    public static let transactionDetailsStatusTitle = NSLocalizedString(
      "wallet.transactionDetailsStatusTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Status",
      comment: "The label for the transaction status, which describes the how far along a transaction is to completing. Appears next to the words such as 'Approved', 'Submitted', 'Pending', etc."
    )
    public static let sent = NSLocalizedString(
      "wallet.sent",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Sent",
      comment: "As in sent cryptocurrency from one asset to another"
    )
    public static let settingsEnableBiometricsTitle = NSLocalizedString(
      "wallet.settingsEnableBiometricsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Allow Biometric Unlock",
      comment: "A label beside the toggle for allowing biometrics to unlock the wallet in wallet settings."
    )
    public static let settingsEnableBiometricsFooter = NSLocalizedString(
      "wallet.settingsEnableBiometricsFooter",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Toggle on if you'd like to use biometrics (Face ID / Touch ID) to unlock your wallet",
      comment: "The footer beneath the toggle for allowing biometrics to unlock the wallet in wallet settings."
    )
    public static let enterPasswordForBiometricsNavTitle = NSLocalizedString(
      "wallet.enterPasswordForBiometricsNavTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enable Biometrics",
      comment: "The navigation bar title displayed on the screen to enter wallet password to enable biometrics unlock from wallet settings."
    )
    public static let enterPasswordForBiometricsTitle = NSLocalizedString(
      "wallet.enterPasswordForBiometricsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Enter your wallet password to enable biometrics for wallet (you'll only be asked for this once)",
      comment: "The title displayed on the screen to enter password to enable biometrics unlock from wallet settings."
    )
    public static let dappsConnectionNotificationTitle = NSLocalizedString(
      "wallet.dappsConnectionNotificationTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This page wants to interact with Brave Wallet",
      comment: "The title of the notification which will prompt at the top of the browser when users are visiting web3 site that is not yet connected with Brave Wallet."
    )
    public static let editPermissionsTitle = NSLocalizedString(
      "wallet.editPermissionsTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Edit Permissions",
      comment: "The title of the detail view to edit the permissions / allowance of an ERC 20 Approve transaction."
    )
    public static let editPermissionsApproveUnlimited = NSLocalizedString(
      "wallet.editPermissionsApproveUnlimited",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unlimited",
      comment: "The value to show when the permissions / allowance of an ERC 20 Approve transaction are the maximum allowance or unlimited spending ammount."
    )
    public static let editPermissionsAllowanceHeader = NSLocalizedString(
      "wallet.editPermissionsAllowanceHeader",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Spend limit permission allows %@ to withdraw and spend up to the following amount:",
      comment: "The header text shown above the rows to selected the allowance for an ERC 20 Approve transaction. The '%@' becomes the name of the account approving the transaction. For example: \"Spend limit permission allows Account 1 to withdraw and spend up to the following amount:\""
    )
    public static let editPermissionsProposedAllowanceHeader = NSLocalizedString(
      "wallet.editPermissionsProposedAllowanceHeader",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Proposed allowance",
      comment: "The header text shown in the row that displays the proposed allowance value."
    )
    public static let editPermissionsCustomAllowanceHeader = NSLocalizedString(
      "wallet.editPermissionsCustomAllowanceHeader",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Set custom allowance",
      comment: "The header text above the field to input a custom allowance value."
    )
    public static let editPermissionsSetUnlimited = NSLocalizedString(
      "wallet.editPermissionsSetUnlimited",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Set Unlimited",
      comment: "The title of the button shown beside the custom allowance input field to make the custom allowance value \"Unlimited\"."
    )
    public static let newSiteConnectScreenTitle = NSLocalizedString(
      "wallet.newSiteConnectScreenTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connect",
      comment: "The transaction edit error alert button which will dismiss the alert."
    )
    public static let newSiteConnectMessage = NSLocalizedString(
      "wallet.newSiteConnectMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connect to Brave Wallet",
      comment: "The message displayed below the dapp's origin url in new site connection screen."
    )
    public static let newSiteConnectFooter = NSLocalizedString(
      "wallet.newSiteConnectFooter",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Only connect with sites you trust.",
      comment: "The footer message displayed above the Next button in new site connection screen."
    )
    public static let newSiteConnectConfirmationMessage = NSLocalizedString(
      "wallet.newSiteConnectConfirmationMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "View the addresses of your permitted accounts (required)",
      comment: "A text displayed below the account address in new site connection confirmation step, in order to make sure users double check the account address they are going to allow the dapp to connect with."
    )
    public static let settingsDefaultBaseCurrencyTitle = NSLocalizedString(
      "wallet.settingsDefaultBaseCurrencyTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Default Base Currency",
      comment: "The title that appears before the current default base currency code. Example: \"Default base currency: USD\""
    )
    public static let addSuggestedTokenTitle = NSLocalizedString(
      "wallet.addSuggestedTokenTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add Suggested Token",
      comment: "The title of the view shown over a dapps website that requests the user add / approve a new token."
    )
    public static let addSuggestedTokenSubtitle = NSLocalizedString(
      "wallet.addSuggestedTokenSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Would you like to import this token?",
      comment: "The subtitle of the view shown over a dapps website that requests the user add / approve a new token, explaining to the user what this request does."
    )
    public static let contractAddressAccessibilityLabel = NSLocalizedString(
      "wallet.contractAddressAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Contract Address",
      comment: "The accessibility label for the contract address of a token / asset."
    )
    public static let switchNetworkTitle = NSLocalizedString(
      "wallet.switchNetworkTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Switch Network",
      comment: "The title of the view shown over a dapps website that requests the user change the current network."
    )
    public static let switchNetworkSubtitle = NSLocalizedString(
      "wallet.switchNetworkSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Allow this site to switch the network?",
      comment: "The subtitle of the view shown over a dapps website that requests permission from the user to change the current network."
    )
    public static let switchNetworkDescription = NSLocalizedString(
      "wallet.switchNetworkDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This will switch the network to a previously added network",
      comment: "The description of the view shown over a dapps website that describes what switching the network will do."
    )
    public static let switchNetworkButtonTitle = NSLocalizedString(
      "wallet.switchNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Switch",
      comment: "The title of the button to approve switching the network on a switch network request from a dapps website."
    )
    public static let addNetworkTitle = NSLocalizedString(
      "wallet.addNetworkTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Add Network",
      comment: "The title of the view shown over a dapps website that requests the user add a new network."
    )
    public static let addNetworkSubtitle = NSLocalizedString(
      "wallet.addNetworkSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Allow this site to add a network?",
      comment: "The subtitle of the view shown over a dapps website that requests permission from the user to add a new network."
    )
    public static let addNetworkDescription = NSLocalizedString(
      "wallet.addNetworkDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This will allow this network to be used within Brave Wallet.",
      comment: "The description of the view shown over a dapps website that describes what adding a new network will do."
    )
    public static let approveNetworkButtonTitle = NSLocalizedString(
      "wallet.approveNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Approve",
      comment: "The title of the button to approve adding a new network on a add new network request from a dapps website."
    )
    public static let networkNameTitle = NSLocalizedString(
      "wallet.networkNameTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Network Name",
      comment: "The title for the label that is shown above the name of the network on a request to switch networks or add a new network from a dapps website."
    )
    public static let networkURLTitle = NSLocalizedString(
      "wallet.networkURLTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Network URL",
      comment: "The label that is shown above the URL for the network on a request to switch networks or add a new network from a dapps website."
    )
    public static let viewDetails = NSLocalizedString(
      "wallet.viewDetails",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "View Details",
      comment: "The title for the button to view details about the network the dapp site is requesting the user switch to, or the network the dapp website is requesting the user add."
    )
    public static let signatureRequestTitle = NSLocalizedString(
      "wallet.signatureRequestTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Signature Requested",
      comment: "A title of the view shown over a dapps website that requests the user sign a message."
    )
    public static let signatureRequestSubtitle = NSLocalizedString(
      "wallet.signatureRequestSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Your signature is being requested",
      comment: "A subtitle of the view shown over a dapps website that requests the user sign a message."
    )
    public static let sign = NSLocalizedString(
      "wallet.sign",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Sign",
      comment: "The title of the button used to sign a message request on the signature request view."
    )
    public static let web3PreferencesSectionTitle = NSLocalizedString(
      "wallet.web3PreferencesSectionTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Web3 preferences",
      comment: "The section title for users to set up preferences for interation with web3 sites."
    )
    public static let web3PreferencesDefaultWallet = NSLocalizedString(
      "wallet.web3PreferencesDefaultWallet",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Default Wallet",
      comment: "The title for the entry displaying the current preferred default wallet is."
    )
    public static let web3PreferencesAllowSiteToRequestAccounts = NSLocalizedString(
      "wallet.web3PreferencesAllowSiteToRequestAccounts",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Allow Sites to Request Accounts",
      comment: "The title for the entry displaying the preferred option to allow web3 sites to rquest account's permission."
    )
    public static let web3PreferencesDisplayWeb3Notifications = NSLocalizedString(
      "wallet.web3PreferencesDisplayWeb3Notifications",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Display Web3 Notifications",
      comment: "The title for the entry displaying the preferred option to display web3 site notifications."
    )
    public static let web3PreferencesManageSiteConnections = NSLocalizedString(
      "wallet.web3PreferencesManageSiteConnections",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Manage Site Connections",
      comment: "The title for the entry that by clicking will direct users to the screen to manage web3 sites account connections, and the title for the Manage Site Connections screen."
    )
    public static let manageSiteConnectionsFilterPlaceholder = NSLocalizedString(
      "wallet.manageSiteConnectionsFilterPlaceholder",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Filter",
      comment: "The filter in the search bar for the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsRemoveAll = NSLocalizedString(
      "wallet.manageSiteConnectionsRemoveAll",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Remove All",
      comment: "The title of the button on the screen to manage web3 sites account connections that will show an alert to remove all connected website permissions."
    )
    public static let manageSiteConnectionsConfirmAlertTitle = NSLocalizedString(
      "wallet.manageSiteConnectionsConfirmAlertTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Remove all permissions?",
      comment: "The title of the alert to confirm the users wants to remove all site connections, shown on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsConfirmAlertMessage = NSLocalizedString(
      "wallet.manageSiteConnectionsConfirmAlertMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This will remove all Wallet connection permissions for all websites.",
      comment: "The message of the alert to confirm the users wants to remove all site connections, shown on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsDetailConfirmAlertMessage = NSLocalizedString(
      "wallet.manageSiteConnectionsDetailConfirmAlertMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This will remove all Wallet connection permissions for this website.",
      comment: "The message of the alert to confirm the users wants to remove all site connections from this specific website, shown after selecting/opening a website on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsConfirmAlertRemove = NSLocalizedString(
      "wallet.manageSiteConnectionsConfirmAlertRemove",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Remove",
      comment: "The title of the confirmation button in the alert to confirm the users wants to remove all site connections, shown on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsAccount = NSLocalizedString(
      "wallet.manageSiteConnectionsAccount",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "%lld %@",
      comment: "The amount of current permitted wallet accounts to the dapp site. It is displayed below the origin url of the dapp site in manage site connections screen. '%lld' refers to a number, %@ is `account` for singular and `accounts` for plural (for example \"1 account\" or \"2 accounts\")"
    )
    public static let manageSiteConnectionsAccountSingular = NSLocalizedString(
      "wallet.manageSiteConnectionsAccountSingular",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "account",
      comment: "The singular word that will be used in `manageSiteConnectionsAccount`."
    )
    public static let manageSiteConnectionsAccountPlural = NSLocalizedString(
      "wallet.manageSiteConnectionsAccountPlural",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "accounts",
      comment: "The plural word that will beused in `manageSiteConnectionsAccount`."
    )
    public static let manageSiteConnectionsDetailHeader = NSLocalizedString(
      "wallet.manageSiteConnectionsDetailHeader",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connected Ethereum Accounts",
      comment: "The header shown above the list of connected accounts for a single website, shown after selecting/opening a website on the screen to manage web3 sites account connections."
    )
    public static let walletTypeNone = NSLocalizedString(
      "wallet.walletTypeNone",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "None",
      comment: "The value shown when selecting the default wallet as none / no wallet in wallet settings."
    )
    public static let walletPanelUnlockWallet = NSLocalizedString(
      "wallet.walletPanelUnlockWallet",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Unlock wallet",
      comment: "The title of the button in wallet panel when wallet is locked. Users can click it to open full screen unlock wallet screen."
    )
    public static let walletPanelSetupWalletDescription = NSLocalizedString(
      "wallet.walletPanelSetupWalletDescription",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Use this panel to securely access web3 and all your crypto assets.",
      comment: "The description for wallet panel when users haven't set up a wallet yet."
    )
    public static let walletFullScreenAccessibilityTitle = NSLocalizedString(
      "wallet.walletFullScreenAccessibilityTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Open wallet in full screen",
      comment: "The label read out when a user is using VoiceOver and highlights the two-arrow button on the wallet panel top left corner."
    )
    public static let editSiteConnectionScreenTitle = NSLocalizedString(
      "wallet.editSiteConnectionScreenTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connections",
      comment: "The navigation title of the screen for users to edit dapps site connection with users' Brave Wallet accounts."
    )
    public static let editSiteConnectionConnectedAccount = NSLocalizedString(
      "wallet.editSiteConnectionConnectedAccount",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "%lld %@ connected",
      comment: "The amount of current permitted wallet accounts to the dapp site. It is displayed below the origin url of the dapp site in edit site connection screen. '%lld' refers to a number, %@ is `account` for singular and `accounts` for plural (for example \"1 account connected\" or \"2 accounts connected\")"
    )
    public static let editSiteConnectionAccountSingular = NSLocalizedString(
      "wallet.editSiteConnectionAccountSingular",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "account",
      comment: "The singular word that will be used in `editSiteConnectionConnectedAccount`."
    )
    public static let editSiteConnectionAccountPlural = NSLocalizedString(
      "wallet.editSiteConnectionAccountPlural",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "accounts",
      comment: "The plural word that will beused in `editSiteConnectionConnectedAccount`."
    )
    public static let editSiteConnectionAccountActionConnect = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionConnect",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connect",
      comment: "The title of the button for users to click so that they connect wallet account to the dapp site(also permission given). It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let editSiteConnectionAccountActionDisconnect = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionDisconnect",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Disconnect",
      comment: "The title of the button for users to click so that they disconnect wallet account to the dapp site(also permission removed). It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let editSiteConnectionAccountActionSwitch = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionSwitch",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Switch",
      comment: "The title of the button for users to click so that they disconnect wallet account to the dapp site(also permission removed). It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let walletPanelConnected = NSLocalizedString(
      "wallet.walletPanelConnected",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connected…",
      comment: "The title of the button for users to click to go to edit site connection screen. This title indicates the user is currently connected his/her wallet account to the dapp. The title will be displayed to the right of a checkmark."
    )
    public static let walletPanelConnect = NSLocalizedString(
      "wallet.walletPanelConnect",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Connect",
      comment: "The title of the button for users to click to go to edit site connection screen. This title indicates the user is currently not connected any his/her wallet account to the dapp."
    )
    public static let getEncryptionPublicKeyRequestTitle = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Public Encryption Key Request",
      comment: "A title of the view shown over a dapps website that requests the users public encryption key."
    )
    public static let getEncryptionPublicKeyRequestSubtitle = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "A DApp is requesting your public encryption key",
      comment: "A subtitle of the view shown over a dapps website that requests the users public encryption key."
    )
    public static let getEncryptionPublicKeyRequestMessage = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "is requesting your wallet's public encryption key. If you consent to providing this key, the site will be able to compose encrypted messages to you.",
      comment: "The text shown beside the URL origin of a get public encryption key request from a dapp site. Ex 'https://brave.com is requesting your wallets public encryption key. If you consent to providing this key, the site will be able to compose encrypted messages to you.'"
    )
    public static let getEncryptionPublicKeyRequestApprove = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestApprove",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Provide",
      comment: "The title of the button to approve a encryption public key request from a dapps website."
    )
    public static let decryptRequestTitle = NSLocalizedString(
      "wallet.decryptRequestTitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Decrypt Request",
      comment: "A title of the view shown over a dapps website that requests the user decrypt a message."
    )
    public static let decryptRequestSubtitle = NSLocalizedString(
      "wallet.decryptRequestSubtitle",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "This DApp would like to read this message to complete your request",
      comment: "A subtitle of the view shown over a dapps website that requests the user decrypt a message."
    )
    public static let decryptRequestApprove = NSLocalizedString(
      "wallet.decryptRequestApprove",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Allow",
      comment: "The title of the button to approve a decrypt request from a dapps website."
    )
    public static let decryptRequestReveal = NSLocalizedString(
      "wallet.decryptRequestReveal",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Reveal",
      comment: "The title of the button to show the message of a decrypt request from a dapps website."
    )
    public static let decryptMessageScreenshotDetectedMessage = NSLocalizedString(
      "wallet.decryptMessageScreenshotDetectedMessage",
      tableName: "BraveWallet",
      bundle: .strings,
      value: "Warning: A screenshot of your message may get backed up to a cloud file service, and be readable by any application with photos access. Brave recommends that you not save this screenshot, and delete it as soon as possible.",
      comment: "The message displayed when the user takes a screenshot of their dapp decrypt request."
    )
  }
}
