// swift-format-ignore-file

// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct Wallet {
    public static let braveWallet = NSLocalizedString(
      "wallet.module",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave Wallet",
      comment:
        "The title shown on the wallet settings page, and the value shown when selecting the default wallet as Brave Wallet in wallet settings."
    )
    public static let wallet = NSLocalizedString(
      "wallet.wallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Wallet",
      comment: "The title shown on the menu to access Brave Wallet"
    )
    public static let web3 = NSLocalizedString(
      "wallet.web3",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Web3",
      comment: "The title shown on the settings menu to Web3 settings."
    )
    public static let otherWalletActionsAccessibilityTitle = NSLocalizedString(
      "wallet.otherWalletActionsAccessibilityTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Other wallet actions",
      comment:
        "The label read out when a user is using VoiceOver and highlights the ellipsis button on the portfolio page"
    )
    public static let portfolioPageTitle = NSLocalizedString(
      "wallet.portfolioPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Portfolio",
      comment: "The title of the portfolio page in the Crypto tab"
    )
    public static let accountsPageTitle = NSLocalizedString(
      "wallet.accountsPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Accounts",
      comment: "The title of the accounts page in the Crypto tab"
    )
    public static let totalBalance = NSLocalizedString(
      "wallet.totalBalance",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Total Balance",
      comment: "A title label that will display total balance of all none-zero balance accounts"
    )
    public static let selectedNetworkAccessibilityLabel = NSLocalizedString(
      "wallet.selectedNetwork",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Selected Network",
      comment: "The accessibility label for the ethereum network picker"
    )
    public static let selectedAccountAccessibilityLabel = NSLocalizedString(
      "wallet.selectedAccount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Selected Account",
      comment: "The accessibility label for the selected account picker"
    )
    public static let assetsTitle = NSLocalizedString(
      "wallet.assetsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Assets",
      comment: "The title which is displayed above a list of assets/tokens"
    )
    public static let nftsTitle = NSLocalizedString(
      "wallet.nftsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "NFTs",
      comment:
        "The title which is displayed above a list of NFTs. 'NFT' is an acronym for Non-Fungible Token."
    )
    public static let transactionsTitle = NSLocalizedString(
      "wallet.transactionsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transactions",
      comment: "The title which is displayed above a list of transactions"
    )
    public static let assetSearchEmpty = NSLocalizedString(
      "wallet.assetSearchEmpty",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No assets found",
      comment:
        "The text displayed when a user uses a query to search for assets that yields no results"
    )
    public static let searchTitle = NSLocalizedString(
      "wallet.searchTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Search assets",
      comment: "The title of the asset search page"
    )
    public static let noAssets = NSLocalizedString(
      "wallet.noAssets",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No Assets",
      comment: "The empty state displayed when the user has no assets associated with an account"
    )
    public static let noAccounts = NSLocalizedString(
      "wallet.noAccounts",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No Accounts",
      comment:
        "The empty state displayed when the user has no accounts associated with a transaction or asset"
    )
    public static let noAccountDescription = NSLocalizedString(
      "wallet.noAccountDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Accounts with a balance will appear here.",
      comment:
        "The empty state description displayed when the user has no accounts associated with a transaction or asset"
    )
    public static let noTransactions = NSLocalizedString(
      "wallet.noTransactions",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No Transactions",
      comment:
        "The empty state displayed when the user has no transactions associated with an account"
    )
    public static let detailsButtonTitle = NSLocalizedString(
      "wallet.detailsButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Details",
      comment:
        "A button title which when pressed displays a new screen with additional details/information"
    )
    public static let hideDetailsButtonTitle = NSLocalizedString(
      "wallet.hideDetailsButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hide Details",
      comment: "A button title which when pressed hides the details screen."
    )
    public static let renameButtonTitle = NSLocalizedString(
      "wallet.renameButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Rename",
      comment:
        "A button on an account screen which when pressed presents a new screen to  rename the account"
    )
    public static let accountDetailsTitle = NSLocalizedString(
      "wallet.accountDetailsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Account Details",
      comment: "The title displayed on the account details screen"
    )
    public static let accountDetailsNameTitle = NSLocalizedString(
      "wallet.accountDetailsNameTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Account Name",
      comment: "The title displayed above a text field that contains the account's name"
    )
    public static let accountDetailsNamePlaceholder = NSLocalizedString(
      "wallet.accountDetailsNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter account name",
      comment: "The placeholder of the text field which allows the user to edit the account's name"
    )
    public static let accountNameLengthError = NSLocalizedString(
      "wallet.accountNameLengthError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Account name must be %lld characters or less",
      comment:
        "The error shown below the account name field when adding or renaming a wallet account if the length is longer than max characters. '%lld' refers to a number (for example \"Account name must be at 30 characters or less\")"
    )
    public static let accountPrivateKey = NSLocalizedString(
      "wallet.accountPrivateKey",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Private Key",
      comment: "A button title for displaying their accounts private key"
    )
    public static let accountRemoveButtonTitle = NSLocalizedString(
      "wallet.accountRemoveButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Remove Account",
      comment: "A button title to trigger deleting a secondary account"
    )
    public static let accountRemoveAlertConfirmation = NSLocalizedString(
      "wallet.accountRemoveAlertConfirmation",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Remove this account?",
      comment: "The title of a confirmation dialog when attempting to remove an account"
    )
    public static let warningAlertConfirmation = NSLocalizedString(
      "wallet.warningAlertConfirmation",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Are you sure?",
      comment:
        "The message of a confirmation dialog when attempting to remove an account. Or the title of a confirmation dialog when attempting to remove all wallet connection for one or more websites"
    )
    public static let accountPrivateKeyDisplayWarning = NSLocalizedString(
      "wallet.accountPrivateKeyDisplayWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Warning: Never share your private key. Anyone with this key can take your assets forever.",
      comment: "A warning message displayed at the top of the Private Key screen"
    )
    public static let copyToPasteboard = NSLocalizedString(
      "wallet.copyToPasteboard",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Tap to copy",
      comment: "A button title that when tapped will copy some data to the users clipboard"
    )
    public static let copiedToPasteboard = NSLocalizedString(
      "wallet.copiedToPasteboard",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Copied",
      comment: "A button title that user has clicked to copy some data to the users clipboard"
    )
    public static let pasteFromPasteboard = NSLocalizedString(
      "wallet.pasteFromPasteboard",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Paste",
      comment:
        "A button title that when tapped will paste some data from the users clipboard to a text field"
    )
    public static let showPrivateKeyButtonTitle = NSLocalizedString(
      "wallet.showPrivateKeyButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Show Private Key",
      comment: "A button title that will make a private key visible on the screen"
    )
    public static let hidePrivateKeyButtonTitle = NSLocalizedString(
      "wallet.hidePrivateKeyButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hide Private Key",
      comment: "A button title that will redact a private key on the screen"
    )
    public static let accountBackup = NSLocalizedString(
      "wallet.accountBackup",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Backup",
      comment:
        "A button title that shows a screen that allows the user to backup their recovery phrase"
    )
    public static let defaultAccountName = NSLocalizedString(
      "wallet.defaultAccountName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ Account %lld",
      comment:
        "The default account name when adding an account and not entering a custom name. '%@' refers to a coin type and '%lld' refers to a number (for example \"Ethereum Account 2\")"
    )
    public static let defaultTestnetAccountName = NSLocalizedString(
      "wallet.defaultTestnetAccountName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ Testnet Account %lld",
      comment:
        "The default account name when adding an account and not entering a custom name. '%@' refers to a coin type and '%lld' refers to a number (for example \"Filecoin Testnet Account 2\")"
    )
    public static let addAccountWithCoinTypeTitle = NSLocalizedString(
      "wallet.addAccountWithCoinTypeTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add %@ Account",
      comment:
        "The title of the add account screen. '%@' will be placed with a coin type title. For example, 'Add Ethereum Account'. "
    )
    public static let addAccountTitle = NSLocalizedString(
      "wallet.addAccountTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add Account",
      comment: "The title of the add account screen."
    )
    public static let add = NSLocalizedString(
      "wallet.addAccountAddButton",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add",
      comment:
        "The title of the button which when tapped will add a new account to the users list of crypto accounts. It will be used as the title of the navigation bar item button on the top right of the add custom token scree."
    )
    public static let failedToImportAccountErrorTitle = NSLocalizedString(
      "wallet.failedToImportAccountErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Failed to import account.",
      comment:
        "The title of an alert when the account the user attempted to import fails for some reason"
    )
    public static let failedToImportAccountErrorMessage = NSLocalizedString(
      "wallet.failedToImportAccountErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Please try again.",
      comment:
        "The message of an alert when the account the user attempted to import fails for some reason"
    )
    public static let importAccountOriginPasswordTitle = NSLocalizedString(
      "wallet.importAccountOriginPasswordTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Origin Password",
      comment: "A title above a text field that is used to enter a password"
    )
    public static let passwordPlaceholder = NSLocalizedString(
      "wallet.passwordPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter password",
      comment: "A placeholder string that will be used on password text fields"
    )
    public static let newPasswordPlaceholder = NSLocalizedString(
      "wallet.newPasswordPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter new password",
      comment:
        "A placeholder string that will be used on password text fields to create a new wallet"
    )
    public static let repeatedPasswordPlaceholder = NSLocalizedString(
      "wallet.repeatedPasswordPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Re-enter password",
      comment: "A placeholder string that will be used on repeat password text fields"
    )
    public static let repeatedPasswordMatch = NSLocalizedString(
      "wallet.repeatedPasswordMatch",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Match!",
      comment:
        "A label will be displayed when the repeat password field input is matched with the password field."
    )
    public static let importAccountSectionTitle = NSLocalizedString(
      "wallet.importAccountSectionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You can create a secondary account by importing your private key.",
      comment: "A title above a text field that will be used to import a users secondary accounts"
    )
    public static let importAccountPlaceholder = NSLocalizedString(
      "wallet.importAccountPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter, paste, or import your private key string file or JSON.",
      comment:
        "A placeholder on a text box for entering the users private key/json data to import accounts"
    )
    public static let importNonEthAccountPlaceholder = NSLocalizedString(
      "wallet.importNonEthAccountPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter or paste your private key.",
      comment: "A placeholder on a text box for entering the users private key to import accounts"
    )
    public static let importButtonTitle = NSLocalizedString(
      "wallet.importButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Import…",
      comment: "A button title that when tapped will display a file import dialog"
    )
    public static let importedCryptoAccountsTitle = NSLocalizedString(
      "wallet.importedCryptoAccountsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Imported Accounts",
      comment: "A title above a list of crypto accounts that are imported"
    )
    public static let secondaryCryptoAccountsSubtitle = NSLocalizedString(
      "wallet.secondaryCryptoAccountsSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Import your external wallet account with a separate seed phrase.",
      comment: "A subtitle above a list of crypto accounts that are imported"
    )
    public static let noSecondaryAccounts = NSLocalizedString(
      "wallet.noSecondaryAccounts",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No secondary accounts.",
      comment: "The empty state shown when you have no imported accounts"
    )
    public static let ethAccountDescription = NSLocalizedString(
      "wallet.ethAccountDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Ethereum + EVM Chains",
      comment: "A description of an Ethereum account, displayed in Accounts tab."
    )
    public static let solAccountDescription = NSLocalizedString(
      "wallet.solAccountDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Solana + SVM Chains",
      comment: "A description of an Solana account, displayed in Accounts tab."
    )
    public static let filAccountDescription = NSLocalizedString(
      "wallet.filAccountDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Filecoin",
      comment: "A description of an Filecoin account, displayed in Accounts tab."
    )
    public static let btcAccountDescription = NSLocalizedString(
      "wallet.btcAccountDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Bitcoin",
      comment: "A description of an Bitcoin account, displayed in Accounts tab."
    )
    public static let exportButtonTitle = NSLocalizedString(
      "wallet.exportButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Export",
      comment: "A button title displayed in a menu to export an account's private key."
    )
    public static let incorrectPasswordErrorMessage = NSLocalizedString(
      "wallet.incorrectPasswordErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Incorrect password",
      comment:
        "The error message displayed when the user enters the wrong password while unlocking the wallet"
    )
    public static let unlockWalletDescription = NSLocalizedString(
      "wallet.unlockWalletDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter password to unlock wallet",
      comment: "The title displayed on the unlock wallet screen"
    )
    public static let unlockWalletButtonTitle = NSLocalizedString(
      "wallet.unlockWalletButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unlock",
      comment:
        "The button title of the unlock wallet button. As in to enter a password and gain access to your wallet's assets."
    )
    public static let restoreWalletButtonTitle = NSLocalizedString(
      "wallet.restoreWalletButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Restore",
      comment:
        "The button title for showing the restore wallet screen. As in to use your recovery phrase to bring a wallet into Brave"
    )
    public static let cryptoTitle = NSLocalizedString(
      "wallet.cryptoTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave Wallet",
      comment: "The title of the crypto tab"
    )
    public static let backupWalletWarningMessage = NSLocalizedString(
      "wallet.backupWalletWarningMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Back up your wallet now to protect your crypto assets and ensure you never lose access.",
      comment:
        "The message displayed on the crypto tab if you have not yet completed the backup process"
    )
    public static let editVisibleAssetsButtonTitle = NSLocalizedString(
      "wallet.editVisibleAssetsButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Edit Visible Assets",
      comment: "The button title for showing the screen to change what assets are visible"
    )
    public static let buy = NSLocalizedString(
      "wallet.buy",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Buy",
      comment: "As in buying cryptocurrency"
    )
    public static let buyDescription = NSLocalizedString(
      "wallet.buyDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Buy crypto with Apple Pay, credit or debit card.",
      comment: "The description of a buy button on the buy/send/swap/deposit modal"
    )
    public static let send = NSLocalizedString(
      "wallet.send",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Send",
      comment: "As in sending cryptocurrency to another account"
    )
    public static let sendDescription = NSLocalizedString(
      "wallet.sendDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Send crypto or transfer from one account to another.",
      comment: "The description of a send button on the buy/send/swap/deposit modal"
    )
    public static let swap = NSLocalizedString(
      "wallet.swap",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap",
      comment: "As in swapping cryptocurrency from one asset to another"
    )
    public static let swapDescription = NSLocalizedString(
      "wallet.swapDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap tokens and assets.",
      comment: "The description of a swap button on the buy/send/swap/deposit modal"
    )
    public static let depositDescription = NSLocalizedString(
      "wallet.depositDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Deposit assets",
      comment: "The description of a swap button on the buy/send/swap/deposit modal"
    )
    public static let more = NSLocalizedString(
      "wallet.more",
      tableName: "BraveWallet",
      bundle: .module,
      value: "More",
      comment:
        "A button title for user to open more option in asset details screen other than buy/send/swap."
    )
    public static let deposit = NSLocalizedString(
      "wallet.deposit",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Deposit",
      comment: "As in depsit some crypto coins to a specific wallet address."
    )
    public static let infoTitle = NSLocalizedString(
      "wallet.infoTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Info",
      comment: "A title above additional information about an asset"
    )
    public static let skipButtonTitle = NSLocalizedString(
      "wallet.skipButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Skip",
      comment: "The button title to skip recovery phrase backup"
    )
    public static let backupWalletTitle = NSLocalizedString(
      "wallet.backupWalletTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Back up your crypto wallet",
      comment: "The title of the backup wallet screen"
    )
    public static let backupWalletSubtitle = NSLocalizedString(
      "wallet.backupWalletSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "In the next step you’ll see a 12-word recovery phrase, which you can use to recover your primary crypto accounts. Save it someplace safe. Your recovery phrase is the only way to regain account access in case of forgotten password, lost or stolen device, or you want to switch wallets.",
      comment: "The subtitle of the backup wallet screen"
    )
    public static let backupWalletDisclaimer = NSLocalizedString(
      "wallet.backupWalletDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "I understand that if I lose my recovery phrase, I won’t be able to access my crypto wallet.",
      comment: "The label next to a toggle which the user must acknowledge"
    )
    public static let backupWalletPasswordPlaceholder = NSLocalizedString(
      "wallet.backupWalletPasswordPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter password to backup wallet",
      comment:
        "The placeholder for the password entry field when a user tries to backup their wallet."
    )
    public static let continueButtonTitle = NSLocalizedString(
      "wallet.continueButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Continue",
      comment: "A button title when a user will continue to the next step of something"
    )
    public static let viewRecoveryPhraseButtonTitle = NSLocalizedString(
      "wallet.viewRecoveryPhraseButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View my recovery phrase",
      comment: "A button title when a user will tap to reveal the recovery phrases."
    )
    public static let backupWalletBackButtonTitle = NSLocalizedString(
      "wallet.backupWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Backup Wallet",
      comment:
        "The title that will be displayed when long-pressing the back button in the navigation bar"
    )
    public static let setupCryptoTitle = NSLocalizedString(
      "wallet.setupCryptoTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Browser native.\nSelf-custody.\nAnd multi-chain.",
      comment: "The title displayed on the 'setup crypto' onboarding screen"
    )
    public static let setupCryptoSubtitle = NSLocalizedString(
      "wallet.setupCryptoSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Take control of your crypto and NFTs. Brave Wallet supports Ethereum, EVM chains, Solana and more.",
      comment: "The subtitle displayed on the 'setup crypto' onboarding screen"
    )
    public static let setupCryptoCreateNewTitle = NSLocalizedString(
      "wallet.setupCryptoCreateNewTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Need a new wallet?",
      comment: "The title displayed in the section for user that need to create a brand new wallet"
    )
    public static let setupCryptoCreateNewSubTitle = NSLocalizedString(
      "wallet.setupCryptoCreateNewSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Get started with Brave Wallet within minutes.",
      comment:
        "The subtitle displayed in the section for user that need to create a brand new wallet"
    )
    public static let setupCryptoRestoreTitle = NSLocalizedString(
      "wallet.setupCryptoRestoreTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Already have a wallet?",
      comment: "The title displayed in the section for user that need to restore an existed wallet"
    )
    public static let setupCryptoRestoreSubTitle = NSLocalizedString(
      "wallet.setupCryptoRestoreSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Import your existing wallet.",
      comment:
        "The subtitle displayed in the section for user that need to restore an existed wallet"
    )
    public static let setupCryptoDisclaimer = NSLocalizedString(
      "wallet.setupCryptoDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "©2023 Brave Software Inc. Brave and the Brave logo are registered trademarks of Brave. Other product names and logos may be trademarks of their respective companies. All rights reserved.",
      comment: "The disclaimer text at the bottom of the first step of onboarding flow."
    )
    public static let setupCryptoButtonTitle = NSLocalizedString(
      "wallet.setupCryptoButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Get Started",
      comment:
        "The button title to continue to the next step on the 'setup crypto' screen. As in to begin the process of creating a wallet/setting up the cryptocurrency feature"
    )
    public static let setupCryptoButtonBackButtonTitle = NSLocalizedString(
      "wallet.setupCryptoButtonBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Welcome",
      comment:
        "The title that will be displayed when long-pressing the back button in the navigation bar. As in the first step of an onboarding process is to welcome a user."
    )
    public static let legalTitle = NSLocalizedString(
      "wallet.legalTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Before We Begin",
      comment:
        "The title of the screen for user to understand the responsibility and the terms of use before setting up a wallet."
    )
    public static let legalDescription = NSLocalizedString(
      "wallet.legalDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "We require that you acknowledge the items below",
      comment:
        "The description of the screen for user to understand the responsibility and the terms of use before setting up a wallet."
    )
    public static let legalUserResponsibility = NSLocalizedString(
      "wallet.legalUserResponsibility",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "I understand this is a self-custody wallet, and that I alone am responsible for any associated funds, assets, or accounts, and for taking appropriate action to secure, protect and backup my wallet. I understand that Brave cannot access my wallet or reverse transactions on my behalf, and that my recovery phrase is the ONLY way to regain access in the event of a lost password, stolen device, or similar circumstance.",
      comment:
        "The responsibility explained in screen for users to check indicates they understand before setting up a wallet."
    )
    public static let legalTermOfUse = NSLocalizedString(
      "wallet.legalTermOfUse",
      tableName: "BraveWallet",
      bundle: .module,
      value: "I have read and agree to the [Terms of Use](%@)",
      comment:
        "For users to check indicates they have read and agree to the Terms of Use before setting up a wallet."
    )
    public static let backupRecoveryPhraseTitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Save Recovery Phrase",
      comment: "The title of the backup recovery phrase screen"
    )
    public static let backupRecoveryPhraseSubtitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Your recovery phrase is the key to access your wallet in case you forget your password or lose your device.\n\nKeep it in a secure place that is not accessible to others and avoid sharing it with anyone.",
      comment: "The subtitle of the backup recovery phrase screen"
    )
    public static let backupRecoveryPhraseBackButtonTitle = NSLocalizedString(
      "wallet.backupRecoveryPhraseBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Recovery Phrase",
      comment:
        "The title that will be displayed when long-pressing the back button in the navigation bar. As in the a list of words to recovery your account on another device/wallet"
    )
    public static let backupSkipButtonTitle = NSLocalizedString(
      "wallet.backupSkipButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "I'll back up later",
      comment:
        "The title of the button that user will tap to skip the verifying recovery phrases step."
    )
    public static let backupSkipPromptTitle = NSLocalizedString(
      "wallet.backupSkipPromptTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "You can skip this step now, but you still need to back up your recovery phrase to ensure account security.",
      comment: "The title on the pop up when user decides to skip backup recovery phrases."
    )
    public static let backupSkipPromptSubTitle = NSLocalizedString(
      "wallet.backupSkipPromptSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "To keep your account secure, make sure to back up your recovery phrase.",
      comment: "The sub-title on the pop up when user decides to skip backup recovery phrases."
    )
    public static let verifyRecoveryPhraseTitle = NSLocalizedString(
      "wallet.verifyRecoveryPhraseTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Let's Check",
      comment: "The title of the screen that user will verify his/her recovery phrase."
    )
    public static let verifyRecoveryPhraseSubTitle = NSLocalizedString(
      "wallet.verifyRecoveryPhraseSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter word in **position %d** from your recovery phrase.",
      comment: "The sub-title of the screen that user will verify his/her recovery phrase."
    )

    public static let verifyRecoveryPhraseError = NSLocalizedString(
      "wallet.verifyRecoveryPhraseError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Recovery phrase doesn't match.",
      comment: "The error message when input phrase does not match the correct one."
    )
    public static let onboardingCompletedTitle = NSLocalizedString(
      "wallet.onbordingCompletedTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You're all set!",
      comment: "The title of the last step of creating a new wallet."
    )
    public static let onboardingCompletedSubTitle = NSLocalizedString(
      "wallet.onbordingCompletedSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Your Brave Wallet is ready to use.",
      comment: "The subtitle of the last step of creating a new wallet."
    )
    public static let onboardingCompletedButtonTitle = NSLocalizedString(
      "wallet.onboardingCompletedButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Go to my portfolio",
      comment:
        "The title of the button in the last step for user to create a new wallet. This will direct users to the wallet portfolio screen."
    )
    public static let restoreWalletBackButtonTitle = NSLocalizedString(
      "wallet.restoreWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Restore Wallet",
      comment:
        "The title that will be displayed when long-pressing the back button in the navigation bar. As to gain access to your assets from a different device"
    )
    public static let restoreWalletPhraseInvalidError = NSLocalizedString(
      "wallet.restoreWalletPhraseInvalidError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Invalid recovery phrase",
      comment:
        "The error message displayed when a user enters an invalid phrase to restore from. By phrase we mean 'recovery phrase' or 'recovery mnemonic'"
    )
    public static let passwordDoesNotMeetRequirementsError = NSLocalizedString(
      "wallet.passwordDoesNotMeetRequirementsError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Passwords must be at least 8 characters.",
      comment:
        "The error message displayed when a user enters a password that does not meet the requirements"
    )
    public static let passwordsDontMatchError = NSLocalizedString(
      "wallet.passwordsDontMatchError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Verified password doesn't match",
      comment:
        "The error displayed when entering two passwords that do not match that are expected to match"
    )
    public static let restoreWalletTitle = NSLocalizedString(
      "wallet.restoreWalletTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Import an Existing Wallet",
      comment: "The title on the restore wallet screen."
    )
    public static let restoreWalletSubtitle = NSLocalizedString(
      "wallet.restoreWalletSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You can paste your entire recovery phrase into any field.",
      comment: "The subtitle on the restore wallet screen."
    )
    public static let restoreWalletPhrasePlaceholder = NSLocalizedString(
      "wallet.restoreWalletPhrasePlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Word #%d",
      comment: "The placeholder on the mneomic/recovery phrase text field of each grid."
    )
    public static let restoreWalletImportFromTwentyFourPhrases = NSLocalizedString(
      "wallet.restoreWalletImportFromLegacyBraveWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "I have a 24-word recovery phrase",
      comment:
        "A button title when 12 recovery-word grids are displayed for users to restore regular wallet. Users can click this button to display the 24 recovery-word grids to restore legacy wallet."
    )
    public static let restoreWalletImportWithTwelvePhrases = NSLocalizedString(
      "wallet.restoreWalletImportFromRegularBraveWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "I have a 12-word recovery phrase",
      comment:
        "A button title when 24 recovery-word grids are displayed for users to restore legacy wallet. Users can click this button to display the 12 recovery-word grids to restore regular wallet."
    )
    public static let restoreWalletShowRecoveryPhrase = NSLocalizedString(
      "wallet.restoreWalletShowRecoveryPhrase",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Show Recovery Phrase",
      comment:
        "A toggle label that will enable or disable visibility of the contents in the recovery phrase text field"
    )
    public static let restoreWalletNewPasswordTitle = NSLocalizedString(
      "wallet.restoreWalletNewPasswordTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "New Password",
      comment: "A title displayed above 2 text fields for entering a new wallet password"
    )
    public static let restoreLegacyBraveWalletToggleLabel = NSLocalizedString(
      "wallet.restoreLegacyBraveWalletToggleTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Import from legacy Brave crypto wallets?",
      comment: "A label for toggle for user to use to indicate importing a legacy brave wallet or not"
    )
    public static let createWalletBackButtonTitle = NSLocalizedString(
      "wallet.createWalletBackButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Create Password",
      comment:
        "The title that will be displayed when long-pressing the back button in the navigation bar. As to make up a new password to create a wallet"
    )
    public static let createWalletTitle = NSLocalizedString(
      "wallet.createWalletTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Create a Password",
      comment: "The title of the create wallet screen"
    )
    public static let createWalletSubTitle = NSLocalizedString(
      "wallet.createWalletSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You'll use this password to access your wallet.",
      comment: "The sub-title of the create wallet screen"
    )
    public static let passwordStatusWeak = NSLocalizedString(
      "wallet.passwordStatusWeak",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Weak",
      comment:
        "A label will be displayed beside input password when it is considered as a weak password"
    )
    public static let passwordStatusMedium = NSLocalizedString(
      "wallet.passwordStatusMedium",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Medium",
      comment:
        "A label will be displayed beside input password when it is considered as a medium password"
    )
    public static let passwordStatusStrong = NSLocalizedString(
      "wallet.passwordStatusStrong",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Strong",
      comment:
        "A label will be displayed beside input password when it is considered as a strong password"
    )
    public static let creatingWallet = NSLocalizedString(
      "wallet.creatingWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Creating Wallet...",
      comment:
        "The title of the creating wallet screen, shown after user enters their password while the wallet is being set up."
    )
    public static let biometricsSetupErrorTitle = NSLocalizedString(
      "wallet.biometricsSetupErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Failed to enable biometrics unlocking.",
      comment: "The title of an alert when the user has an error setting up biometric unlock"
    )
    public static let biometricsSetupErrorMessage = NSLocalizedString(
      "wallet.biometricsSetupErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "There was an error while trying to enable biometrics unlocking. Please try again later.",
      comment: "The message of an alert when the user has an error setting up biometric unlock"
    )
    public static let settingsResetButtonTitle = NSLocalizedString(
      "wallet.settingsResetButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Reset and Clear Wallet Data",
      comment:
        "The title of a button that will reset the wallet. As in to erase the users wallet from the device"
    )
    public static let settingsResetWalletAlertTitle = NSLocalizedString(
      "wallet.settingsResetWalletAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Are you sure you want to reset Brave Wallet?",
      comment:
        "The title the confirmation dialog when resetting the wallet. As in to erase the users wallet from the device"
    )
    public static let settingsResetWalletAlertMessage = NSLocalizedString(
      "wallet.settingsResetWalletAlertMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "This action cannot be reversed. Your recovery phrase is the only way to regain account access to your crypto assets.",
      comment: "The message the confirmation dialog when resetting the wallet."
    )
    public static let settingsResetWalletAlertButtonTitle = NSLocalizedString(
      "wallet.settingsResetWalletAlertButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Reset Wallet",
      comment:
        "The title of a button that will reset the wallet. As in to erase the users wallet from the device"
    )
    public static let dateIntervalHour = NSLocalizedString(
      "wallet.dateIntervalHour",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1H",
      comment:
        "An abbreivated form of \"1 Hour\" used to describe what range of data to show on the graph (past hour)"
    )
    public static let dateIntervalHourAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalHourAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1 Hour",
      comment: "Describes what range of data to show on the graph (past hour)"
    )
    public static let dateIntervalDay = NSLocalizedString(
      "wallet.dateIntervalDay",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1D",
      comment:
        "An abbreivated form of \"1 Day\" used to describe what range of data to show on the graph (past day)"
    )
    public static let dateIntervalDayAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalDayAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1 Day",
      comment: "Describes what range of data to show on the graph (past day)"
    )
    public static let dateIntervalWeek = NSLocalizedString(
      "wallet.dateIntervalWeek",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1W",
      comment:
        "An abbreivated form of \"1 Week\" used to describe what range of data to show on the graph (past week)"
    )
    public static let dateIntervalWeekAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalWeekAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1 Week",
      comment: "Describes what range of data to show on the graph (past week)"
    )
    public static let dateIntervalMonth = NSLocalizedString(
      "wallet.dateIntervalMonth",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1M",
      comment:
        "An abbreivated form of \"1 Month\" used to describe what range of data to show on the graph (past month)"
    )
    public static let dateIntervalMonthAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalMonthAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1 Month",
      comment: "Describes what range of data to show on the graph (past month)"
    )
    public static let dateIntervalThreeMonths = NSLocalizedString(
      "wallet.dateIntervalThreeMonths",
      tableName: "BraveWallet",
      bundle: .module,
      value: "3M",
      comment:
        "An abbreivated form of \"3 Months\" used to describe what range of data to show on the graph (past 3 months)"
    )
    public static let dateIntervalThreeMonthsAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalThreeMonthsAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "3 Months",
      comment: "Describes what range of data to show on the graph (past 3 months)"
    )
    public static let dateIntervalYear = NSLocalizedString(
      "wallet.dateIntervalYear",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1Y",
      comment:
        "An abbreivated form of \"1 Year\" used to describe what range of data to show on the graph (past year)"
    )
    public static let dateIntervalYearAccessibilityLabel = NSLocalizedString(
      "wallet.dateIntervalYearAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "1 Year",
      comment: "Describes what range of data to show on the graph (past year)"
    )
    public static let dateIntervalAll = NSLocalizedString(
      "wallet.dateIntervalAll",
      tableName: "BraveWallet",
      bundle: .module,
      value: "All",
      comment: "Describes what range of data to show on the graph (all data available)"
    )
    public static let swapCryptoFromTitle = NSLocalizedString(
      "wallet.swapCryptoFromTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "From",
      comment:
        "A title above the cryptocurrency token/asset you are swapping from. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let swapCryptoToTitle = NSLocalizedString(
      "wallet.swapCryptoToTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "To",
      comment:
        "A title above the cryptocurrency token/asset you are swapping to. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let swapCryptoAmountTitle = NSLocalizedString(
      "wallet.swapCryptoAmountTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter amount of %@ to swap",
      comment:
        "A title above the amount of asset you want to swap. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapCryptoAmountReceivingTitle = NSLocalizedString(
      "wallet.swapCryptoAmountReceivingTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Amount receiving in %@ (estimated)",
      comment:
        "A title above the amount of asset you will receive from the swap. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let swapOrderTypeLabel = NSLocalizedString(
      "wallet.swapOrderTypeLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Order Type",
      comment: "The type of order you want to place. Options are: 'Market' and 'Limit'"
    )
    public static let swapLimitOrderType = NSLocalizedString(
      "wallet.swapLimitOrderType",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Limit",
      comment:
        "The 'Limit' order type. Limit orders only execute when the price requirements are met"
    )
    public static let swapMarketOrderType = NSLocalizedString(
      "wallet.swapMarketOrderType",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Market",
      comment:
        "The 'Market' order type. Market orders execute immediately based on the price at the time of the order."
    )
    public static let today = NSLocalizedString(
      "wallet.today",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Today",
      comment: "A label appended after a certain dollar or percent change. Example: 'Up 1.4% Today'"
    )
    public static let selectAccountTitle = NSLocalizedString(
      "wallet.selectAccountTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select Account",
      comment:
        "The title of the account selection screen. Will show above a list of accounts the user may pick from"
    )
    public static let assetDetailSubtitle = NSLocalizedString(
      "wallet.assetDetailSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ Price (%@)",
      comment:
        "A subtitle on the asset details screen that uses the name and symbol. Example: Basic Attention Token Price (BAT)"
    )
    public static let biometricsSetupTitle = NSLocalizedString(
      "wallet.biometricsSetupTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unlock Brave Wallet with %@",
      comment:
        "The title shown when a user is asked if they would like to setup biometric unlock. `%@` will be replaced with the biometric type name of the current device."
    )
    public static let biometricsSetupSubTitle = NSLocalizedString(
      "wallet.biometricsSetupSubTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Would you like to use %@ to unlock Brave Wallet?",
      comment:
        "The sub-title shown when a user is asked if they would like to setup biometric unlock. `%@` will be replaced with the biometric type name of the current device."
    )
    public static let biometricsSetupEnableButtonTitle = NSLocalizedString(
      "wallet.biometricsSetupEnableButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Use %@",
      comment:
        "The button title that enables the biometric unlock feature. %@` will be replaced with the biometric type name of the current device."
    )
    public static let biometricsSetupFaceId = NSLocalizedString(
      "wallet.biometricsSetupFaceId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Face ID",
      comment: "Type of biometrics. This will be used in Biometric setup screen."
    )
    public static let biometricsSetupTouchId = NSLocalizedString(
      "wallet.biometricsSetupTouchId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Touch ID",
      comment: "Type of biometrics. This will be used in Biometric setup screen."
    )
    public static let copyAddressButtonTitle = NSLocalizedString(
      "wallet.copyAddressButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Copy Address",
      comment:
        "The button title that appears when long-pressing a wallet address that will copy said address to the users clipboard"
    )
    public static let autoLockTitle = NSLocalizedString(
      "wallet.autoLockTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Automatically Lock After",
      comment:
        "The title that appears before an auto-lock interval. Example: Automatically lock after 5 minutes"
    )
    public static let autoLockFooter = NSLocalizedString(
      "wallet.autoLockFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value: "The number of minutes to wait until the Brave Wallet is automatically locked",
      comment: "The footer beneath the auto-lock title and interval duration"
    )
    public static let enterAmount = NSLocalizedString(
      "wallet.enterAmount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter amount",
      comment:
        "The header title for the textField users will input the dollar value of the crypto they want to buy"
    )
    public static let amountInCurrency = NSLocalizedString(
      "wallet.amountInCurrency",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Amount in %@",
      comment:
        "A placeholder on a text field to describe an amount of some currency. '%@' will be replaced with a currency code such as 'USD' or 'BAT'"
    )
    public static let purchaseMethodButtonTitle = NSLocalizedString(
      "wallet.purchaseMethodButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select purchase method",
      comment: "The title of the button for users to click to select a purchase method."
    )
    public static let sendCryptoFromTitle = NSLocalizedString(
      "wallet.sendCryptoFromTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "From",
      comment:
        "A title above the cryptocurrency token/asset you are sending from. For example this would appear over a cell that has the 'BAT' token selected"
    )
    public static let sendCryptoAmountTitle = NSLocalizedString(
      "wallet.sendCryptoAmountTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter amount of %@ to send",
      comment:
        "A title above the amount of asset you want to send. '%@' will be replaced with a token symbol such as 'ETH' or 'BAT'"
    )
    public static let sendCryptoToTitle = NSLocalizedString(
      "wallet.sendCryptoToTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "To",
      comment:
        "A title above the address you want to send to. For example this would appear over a cell that has the 'OxFCdf***DDee' with a clipboard icon and a qr-code icon on the right hand side"
    )
    public static let sendToCryptoAddressPlaceholder = NSLocalizedString(
      "wallet.sendToCryptoAddressPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter address",
      comment: "A placeholder of the address text field."
    )
    public static let scanQRCodeAccessibilityLabel = NSLocalizedString(
      "wallet.scanQRCodeAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Scan QR code",
      comment:
        "A description for a QR code icon which brings up the camera to read ETH addresses encoded as QR codes"
    )
    public static let sendCryptoSendButtonTitle = NSLocalizedString(
      "wallet.sendCryptoSendButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Send",
      comment:
        "The title of the button for users to click when they want to send the sending-transaction"
    )
    public static let sendCryptoSendError = NSLocalizedString(
      "wallet.sendCryptoSendError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "We currently cannot proceed with your transaction",
      comment:
        "The error message will appear when there is any error occurs during unpproved transaction"
    )
    public static let swapCryptoUnsupportNetworkTitle = NSLocalizedString(
      "wallet.swapCryptoUnsupportNetworkTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unsupported chain",
      comment:
        "The title below account picker when user has selected a test network to swap cryptos"
    )
    public static let swapCryptoUnsupportNetworkDescription = NSLocalizedString(
      "wallet.swapCryptoUnsupportNetworkBody",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave doesn't currently support swap on the %@. Please switch to a supported chain.",
      comment:
        "The description of where user will see once a test network has been picked in swap screen. '%@' will be replaced with a network such as 'Rinkeby Test Network' or 'Ropsten Test Network'"
    )
    public static let swapCryptoSlippageTitle = NSLocalizedString(
      "wallet.swapCryptoSlippageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Slippage Tolerance",
      comment:
        "The title for Slippage tolerance field. User will choose the tolerance for slippage as a percentage value"
    )
    public static let swapCryptoSwapButtonTitle = NSLocalizedString(
      "wallet.swapCryptoSwapButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap",
      comment:
        "The title of the button for users to click when they want to swap between two cryptos"
    )
    public static let swapCryptoMarketPriceTitle = NSLocalizedString(
      "wallet.swapCryptoMarketPriceTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Market Price in %@",
      comment:
        "The title of the field for display the market price of the crypto that user chooses to swap from. The title lives above the price label. '%@' will be replaced with the symbol of the crypto that users choose to swap from as 'ETH' or 'BAT'"
    )
    public static let refreshMarketPriceLabel = NSLocalizedString(
      "wallet.refreshMarketPriceLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Refresh market price",
      comment:
        "A description for a refresh icon that when pressed receives a new snap quote for the currently swap assets"
    )
    public static let swapSelectedTokens = NSLocalizedString(
      "wallet.swapSelectedTokens",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap selected tokens",
      comment:
        "An accessibility message for the swap button below from amount shortcut grids for users to swap the two selected tokens."
    )
    public static let braveFeeLabel = NSLocalizedString(
      "wallet.braveFeeLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave Fee: %@",
      comment:
        "The title for Brave Fee label in Swap. The fee percentage is displayed beside the label."
    )
    public static let protocolFeeLabel = NSLocalizedString(
      "wallet.protocolFeeLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Protocol Fee: %@",
      comment:
        "The title for Protocol Fee label in Swap. The fee percentage is displayed beside the label."
    )
    public static let braveSwapFree = NSLocalizedString(
      "wallet.braveSwapFree",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Free",
      comment:
        "The text beside the striked-through percentage Brave would normally charge for a swap."
    )
    public static let transactionCount = NSLocalizedString(
      "wallet.transactionCount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%lld of %lld",
      comment:
        "Displays the number of transactions and the current transaction that you are viewing when confirming or rejecting multiple transactions. Each '%lld' will be replaced by a number, for example: '1 of 4'"
    )
    public static let next = NSLocalizedString(
      "wallet.next",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Next",
      comment:
        "1. A button title next indicating the user to go to the next transaction. Will sit next to a label such as \"1 of 4\" where tapping next would move them to the second transaction. 2. Title of the button for users to click to the next step during the process of dapp permission requests."
    )
    public static let transactionFromToAccessibilityLabel = NSLocalizedString(
      "wallet.transactionFromToAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "From: %@. To: %@",
      comment:
        "A VoiceOver label that will be read out when a user focuses a transactions \"from address\" and \"to address\" labels. \"%@\" will be replaced with either an account name or a truncated ethereum address such as \"Account 1\" or \"0x1234***3003\""
    )
    public static let confirmationViewModeTransaction = NSLocalizedString(
      "wallet.confirmationViewModeTransaction",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction",
      comment:
        "One of the picker options while confirming a transaction. When selected it displays a summary of the transaction such as value, gas fee, and totals"
    )
    public static let confirmationViewModeDetails = NSLocalizedString(
      "wallet.confirmationViewModeDetails",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Details",
      comment:
        "One of the picker options while confirming a transaction. When selected it displays a transactions function details such as underlying data"
    )
    public static let confirmationViewEditPermissions = NSLocalizedString(
      "wallet.confirmationViewEditPermissions",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Edit permissions",
      comment:
        "The title shown on the button to edit permissions / the allowance for an ERC 20 Approve transaction while confirming a transaction."
    )
    public static let confirmationViewAllowSpendTitle = NSLocalizedString(
      "wallet.confirmationViewAllowSpendTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allow this app to spend your %@?",
      comment:
        "The title shown on transaction confirmation for an ERC 20 Approve transaction where the '%@' will be the name of the symbol being approved. For example: \"Allow this app to spend your DAI?\""
    )
    public static let confirmationViewAllowSpendSubtitle = NSLocalizedString(
      "wallet.confirmationViewAllowSpendSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "By granting this permission, you are allowing this app to withdraw your %@ and automate transactions for you.",
      comment:
        "The subtitle shown on transaction confirmation for an ERC 20 Approve transaction where the '%@' will be the name of the symbol being approved. For example: \"By granting this permission, you are allowing this app to withdraw your DAI and automate transactions for you.\""
    )
    public static let confirmationViewUnlimitedWarning = NSLocalizedString(
      "wallet.confirmationViewUnlimitedWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unlimited approval requested",
      comment:
        "The warning shown on transaction confirmation for an ERC 20 Approve transaction when the proposed allowance is unlimited."
    )
    public static let confirmationViewSolSplTokenAccountCreationWarning = NSLocalizedString(
      "wallet.confirmationViewSolSplTokenAccountCreationWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "The associated token account does not exist yet. A small amount of SOL will be spent to create and fund it.",
      comment:
        "The warning shown on transaction confirmation for an Solana SPL token transaction that has not yet created an associated token account."
    )
    public static let confirmationViewSolAccountOwnershipChangeWarningTitle = NSLocalizedString(
      "wallet.confirmationViewSolAccountOwnershipChangeWarningTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Account ownership change requested",
      comment:
        "The warning title shown on transaction confirmation for an Solana Dapp request that will reassign ownership of the account to a new program."
    )
    public static let confirmationViewSolAccountOwnershipChangeWarning = NSLocalizedString(
      "wallet.confirmationViewSolAccountOwnershipChangeWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "This transaction will reassign ownership of the account to a new program. This action is irreversible and may result in loss of funds.",
      comment:
        "The warning shown on transaction confirmation for an Solana Dapp request that will reassign ownership of the account to a new program."
    )
    public static let confirmationViewCurrentAllowance = NSLocalizedString(
      "wallet.confirmationViewCurrentAllowance",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Current Allowance",
      comment:
        "The label shown beside the current allowance for a ERC20 approve transaction where the current allowance is a number followed by the symbol name. For example, \"Current Allowance  100 DAI\"."
    )
    public static let gasFee = NSLocalizedString(
      "wallet.gasFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Gas Fee",
      comment:
        "A title displayed beside a number describing the cost of the transaction in ETH which is called Gas"
    )
    public static let transactionFee = NSLocalizedString(
      "wallet.transactionFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Fee",
      comment:
        "A title displayed beside a number describing the cost of the transaction in SOL for any Solana transaction"
    )
    public static let editButtonTitle = NSLocalizedString(
      "wallet.editButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Edit",
      comment:
        "A button title displayed under a Gas Fee title that allows the user to adjust the gas fee/transactions priority, on accounts tab for editing an account name, etc."
    )
    public static let total = NSLocalizedString(
      "wallet.total",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Total",
      comment:
        "A title displayed beside a number describing the total amount of gas and ETH that will be transferred"
    )
    public static let amountAndGas = NSLocalizedString(
      "wallet.amountAndGas",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Amount + Gas",
      comment:
        "A title displayed above two numbers (the amount and gas) showing the user the breakdown of the amount transferred and gas fee. The \"+\" is a literal plus as the label below will show such as \"0.004 ETH + 0.00064 ETH\""
    )
    public static let amountAndFee = NSLocalizedString(
      "wallet.amountAndFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Amount + Fee",
      comment:
        "A title displayed above two numbers (the amount and transaction fee) showing the user the breakdown of the amount transferred and transaction fee for any Solana transaction. The \"+\" is a literal plus as the label below will show such as \"0.004 SOL + 0.00064 SOL\""
    )
    public static let inputDataPlaceholder = NSLocalizedString(
      "wallet.inputDataPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No data.",
      comment: "A label shown inside of a box when there is no input data for a given transaction"
    )
    public static let inputDataPlaceholderTx = NSLocalizedString(
      "wallet.inputDataPlaceholderTx",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Function Type:\n%@",
      comment:
        "A label shown inside of a box when there is no input data for a given transaction"
    )
    public static let rejectAllTransactions = NSLocalizedString(
      "wallet.rejectAllTransactions",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Reject %d Transactions",
      comment:
        "A button title that allows the user to reject all unapproved transactions at once. %d will be replaced with a number, example: Reject 4 Transactions"
    )
    public static let confirmTransactionTitle = NSLocalizedString(
      "wallet.confirmTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Confirm Transaction",
      comment: "The title of the transaction confirmation panel UI."
    )
    public static let confirmTransactionsTitle = NSLocalizedString(
      "wallet.confirmTransactionsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Confirm Transactions",
      comment:
        "The title of the transaction confirmation panel UI when there are multiple transactions to confirm"
    )
    public static let confirm = NSLocalizedString(
      "wallet.confirm",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Confirm",
      comment:
        "1. A button title to confirm a transaction. It is shown below details about a given transaction. 2. A button title to confirm to grant permission of a dapp connect request. It is displayed at the bottom of last step to grant permission in new site connection screen."
    )
    public static let rejectTransactionButtonTitle = NSLocalizedString(
      "wallet.rejectTransactionButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Reject",
      comment:
        "A button title to reject a transaction. It is shown below details about a given transaction"
    )
    public static let insufficientFunds = NSLocalizedString(
      "wallet.insufficientFunds",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Insufficient funds",
      comment:
        "An error message displayed when the user does not have enough funds to make or confirm a transaction"
    )
    public static let gasFeeDisclaimer = NSLocalizedString(
      "wallet.gasFeeDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "While not a guarantee, miners will likely prioritize your transaction earlier if you pay a higher fee.",
      comment: "A disclaimer shown above the UI to select a gas fee"
    )
    public static let gasFeePredefinedLimitLow = NSLocalizedString(
      "wallet.gasFeePredefinedLimitLow",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Low",
      comment:
        "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom"
    )
    public static let gasFeePredefinedLimitOptimal = NSLocalizedString(
      "wallet.gasFeePredefinedLimitOptimal",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Optimal",
      comment:
        "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom"
    )
    public static let gasFeePredefinedLimitHigh = NSLocalizedString(
      "wallet.gasFeePredefinedLimitHigh",
      tableName: "BraveWallet",
      bundle: .module,
      value: "High",
      comment:
        "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom"
    )
    public static let gasFeeCustomOption = NSLocalizedString(
      "wallet.gasFeeCustomOption",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Custom",
      comment:
        "An option for the user to pick when selecting some predefined gas fee limits. The options are Low, Optimal, High and Custom. This option allows the user to specify gas fee details themselves"
    )
    public static let gasCurrentBaseFee = NSLocalizedString(
      "wallet.gasCurrentBaseFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Current base fee",
      comment:
        "The base cost of the gas fee before adjustments by the user. It will be shown next to a Gwei amount"
    )
    public static let gasAmountLimit = NSLocalizedString(
      "wallet.gasAmountLimit",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Gas amount limit",
      comment: "A title above a text field for inputting the gas amount limit"
    )
    public static let perGasTipLimit = NSLocalizedString(
      "wallet.perGasTipLimit",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Per-gas tip limit (Gwei)",
      comment: "A title above a text field for inputting the per-gas tip limit in Gwei"
    )
    public static let perGasPriceLimit = NSLocalizedString(
      "wallet.perGasPriceLimit",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Per-gas price limit (Gwei)",
      comment: "A title above a text field for inputting the per-gas price limit in Gwei"
    )
    public static let maximumGasFee = NSLocalizedString(
      "wallet.maximumGasFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Maximum fee",
      comment:
        "The highest the user will pay in a gas fee based on the entered gas fee details or predefined option. It is displayed above the amount"
    )
    public static let saveButtonTitle = NSLocalizedString(
      "wallet.saveButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Save",
      comment:
        "A button title for saving the users selected gas fee options. Or to save a custom nonce value. Or to save a custom network"
    )
    public static let maxPriorityFeeTitle = NSLocalizedString(
      "wallet.maxPriorityFeeTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Max Priority Fee",
      comment: "The title of the edit gas fee screen for EIP-1559 transactions"
    )
    public static let insufficientBalance = NSLocalizedString(
      "wallet.insufficientBalance",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Insufficient balance",
      comment:
        "An error message when there is no insufficient balance for swapping. It will be displayed as the title of the disabled swap button at the bottom in the Swap Screen."
    )
    public static let insufficientFundsForGas = NSLocalizedString(
      "wallet.insufficientFundsForGas",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Insufficient funds for gas",
      comment:
        "An error message when there is no insufficient funds for gas fee. It will be displayed as the title of the disabled swap button at the bottom in the Swap Screen."
    )
    public static let activateToken = NSLocalizedString(
      "wallet.activateToken",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Activate Token %@",
      comment:
        "The title of the button at the bottom of Swap Screen, when the sell token is erc20 and it has not been activated its allowance. %@ will be replaced with the sell token's symbol such as 'DAI' or 'USDC'"
    )
    public static let insufficientLiquidity = NSLocalizedString(
      "wallet.insufficientLiquidity",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Insufficient liquidity",
      comment:
        "An error message displayed when the user doesn't have enough liquidity to proceed with a transaction."
    )
    public static let unknownError = NSLocalizedString(
      "wallet.unknownError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unknown error",
      comment: "An error message displayed when an unspecified problem occurs."
    )
    public static let transactionSummaryFee = NSLocalizedString(
      "wallet.transactionSummaryFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Fee: %@ %@ (%@)",
      comment:
        "A transactions gas fee. The first '%@' becomes the fee amount, the second '%@' becomes the symbol for the fee's currency and the last '%@' becomes the fiat amount. For example: \"Fee: 0.0054 ETH ($22.44)\""
    )
    public static let transactionApproveSymbolTitle = NSLocalizedString(
      "wallet.transactionApproveSymbolTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approved %@ %@",
      comment:
        "The title shown for ERC20 approvals. The first '%@' becomes the  amount, the second '%@' becomes the symbol for the cryptocurrency. For example: \"Approved 150.0 BAT\""
    )
    public static let transactionApprovalTitle = NSLocalizedString(
      "wallet.transactionApprovalTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approved",
      comment:
        "The title shown for ERC20 approvals when the user doesn't have the visible asset added"
    )
    public static let transactionSwappedTitle = NSLocalizedString(
      "wallet.transactionSwappedTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swapped %@ %@ to %@ %@",
      comment:
        "A title shown for a swap transaction. The first '%@' becomes the from amount, the second '%@' becomes the symbol for the from currency, the third '%@' becomes the to amount, and the last '%@' becomes the to symbol. For example: \"Swapped 0.0054 ETH to 1.5 DAI\""
    )
    public static let transactionSendTitle = NSLocalizedString(
      "wallet.transactionSendTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sent %@ %@ (%@)",
      comment:
        "A title shown for a send transaction. The first '%@' becomes the  amount, the second '%@' becomes the symbol for the cryptocurrency and the last '%@' becomes the fiat amount. For example: \"Sent 0.0054 ETH ($22.44)\""
    )
    public static let transactionUnknownSendTitle = NSLocalizedString(
      "wallet.transactionUnknownSendTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sent %@",
      comment:
        "A title shown for a erc 20 transfer. The first '%@' becomes the symbol for the cryptocurrency For example: \"Sent ETH\""
    )
    public static let viewOnBlockExplorer = NSLocalizedString(
      "wallet.viewOnBlockExplorer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View on block explorer",
      comment:
        "A button title to view a given transaction on the block explorer for the current network/chain."
    )
    public static let transactionStatusConfirmed = NSLocalizedString(
      "wallet.transactionStatusConfirmed",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Confirmed",
      comment: "A status that explains that the transaction has been completed/confirmed"
    )
    public static let transactionStatusApproved = NSLocalizedString(
      "wallet.transactionStatusApproved",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approved",
      comment: "A status that explains that the transaction has been approved by the user"
    )
    public static let transactionStatusRejected = NSLocalizedString(
      "wallet.transactionStatusRejected",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Rejected",
      comment: "A status that explains that the transaction has been rejected by the user"
    )
    public static let transactionStatusUnapproved = NSLocalizedString(
      "wallet.transactionStatusUnapproved",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unapproved",
      comment: "A status that explains that a transaction has not yet been approved"
    )
    public static let transactionStatusSubmitted = NSLocalizedString(
      "wallet.transactionStatusSubmitted",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Submitted",
      comment: "A status that explains that the transaction has been submitted to the blockchain"
    )
    public static let transactionStatusDropped = NSLocalizedString(
      "wallet.transactionStatusDropped",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Dropped",
      comment: "A status that explains that the transaction has been dropped due to some error"
    )
    public static let transactionStatusError = NSLocalizedString(
      "wallet.transactionStatusError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Error",
      comment: "A status that explains that the transaction failed due to some error"
    )
    public static let transactionStatusSigned = NSLocalizedString(
      "wallet.transactionStatusSigned",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Signed",
      comment: "A status that explains that the transaction has been signed."
    )
    public static let transactionStatusUnknown = NSLocalizedString(
      "wallet.transactionStatusUnknown",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unknown",
      comment: "A transaction status that the app currently does not support displaying"
    )
    public static let customTokenNetworkHeader = NSLocalizedString(
      "wallet.customTokenNetworkHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select network",
      comment:
        "A title that will be displayed on top of the text field for users to choose a network they are willing to add the custom asset in."
    )
    public static let customTokenNetworkButtonTitle = NSLocalizedString(
      "wallet.customTokenNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select network",
      comment:
        "A title for the btton that users can click to choose the network they are willing to add custom asset in."
    )
    public static let customTokenTitle = NSLocalizedString(
      "wallet.customTokenTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add Custom Asset",
      comment: "The title displayed on the add custom token screen"
    )
    public static let tokenName = NSLocalizedString(
      "wallet.tokenName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token name",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token name"
    )
    public static let enterTokenName = NSLocalizedString(
      "wallet.enterTokenName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter token name",
      comment: "A placeholder for the text field that users will input the custom token name"
    )
    public static let tokenAddress = NSLocalizedString(
      "wallet.tokenAddress",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token address",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token address"
    )
    public static let tokenMintAddress = NSLocalizedString(
      "wallet.tokenMintAddress",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Mint address",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token mint address"
    )
    public static let enterAddress = NSLocalizedString(
      "wallet.enterAddress",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter address",
      comment: "A placeholder for the text field that users will input the custom token address"
    )
    public static let tokenSymbol = NSLocalizedString(
      "wallet.tokenSymbol",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token symbol",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token symbol"
    )
    public static let enterTokenSymbol = NSLocalizedString(
      "wallet.enterTokenSymbol",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter token symbol",
      comment: "A placeholder for the text field that users will input the custom token symbol"
    )
    public static let decimalsPrecision = NSLocalizedString(
      "wallet.decimalsPrecision",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Decimals of precision",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token's decimals of precision"
    )
    public static let addCustomTokenAdvanced = NSLocalizedString(
      "wallet.addCustomTokenAdvanced",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Advanced",
      comment:
        "A title of an initally hidden section in Add custom asset screen for users to input icon url and coingecko id."
    )
    public static let addCustomTokenIconURL = NSLocalizedString(
      "wallet.addCustomTokenIconURL",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Icon URL",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token's icon URL."
    )
    public static let enterTokenIconURL = NSLocalizedString(
      "wallet.enterTokenIconURL",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter token icon URL",
      comment: "A placeholder for the text field that users will input the custom token icon URL"
    )
    public static let addCustomTokenCoingeckoId = NSLocalizedString(
      "wallet.addCustomTokenCoingeckoId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Coingecko ID",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token's coingecko ID."
    )
    public static let enterTokenCoingeckoId = NSLocalizedString(
      "wallet.enterTokenCoingeckoId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter token Coingecko ID",
      comment:
        "A placeholder for the text field that users will input the custom token Coingecko ID."
    )
    public static let addCustomTokenId = NSLocalizedString(
      "wallet.addCustomTokenId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token ID (only for ERC721)",
      comment:
        "A title that will be displayed on top of the text field for users to input the custom token's ID."
    )
    public static let addCustomTokenTitle = NSLocalizedString(
      "wallet.addCustomTokenTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token",
      comment:
        "A title of one segment on top of Add Custom Assets screen, which is default selected. Users would need to select this segment if they are willing to add a custom fungible token."
    )
    public static let addCustomNFTTitle = NSLocalizedString(
      "wallet.addCustomNFTTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "NFT",
      comment:
        "A title of one segment on top of Add Custom Assets screen. Users would need to select this segment if they are willing to add a custom non-fungible token."
    )
    public static let enterTokenId = NSLocalizedString(
      "wallet.enterTokenId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter Token ID",
      comment: "A placeholder for the text field that users will input the custom token's ID."
    )
    public static let addCustomTokenErrorTitle = NSLocalizedString(
      "wallet.addCustomTokenErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Cannot add custom token",
      comment:
        "The title of the error pop up when there is an error occurs during the process of adding a custom token."
    )
    public static let addCustomTokenErrorMessage = NSLocalizedString(
      "wallet.addCustomTokenErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Failed to add custom token, please try again.",
      comment:
        "The message of the error pop up when there is an error occurs during the process of adding a custom token."
    )
    public static let removeCustomTokenErrorTitle = NSLocalizedString(
      "wallet.removeCustomTokenErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Cannot remove custom token",
      comment:
        "The title of the error pop up when there is an error occurs during the process of removing a custom token."
    )
    public static let removeCustomTokenErrorMessage = NSLocalizedString(
      "wallet.removeCustomTokenErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Please verify this is a new custom asset, check your internet connection, and try again.",
      comment:
        "The message in the error pop up when there is an error occurs during the process of removing a custom token."
    )
    public static let addCustomAsset = NSLocalizedString(
      "wallet.addCustomAsset",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add custom asset",
      comment:
        "The title of the button that is located in the same area of the assets list header but on the right side. Users will click it and go to add custom asset screen."
    )
    public static let delete = NSLocalizedString(
      "wallet.delete",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Delete",
      comment:
        "The title of the option inside the context menu for custom asset row in edit user asset screen. Or the title of the option inside the context menu for custom network row which is not being currently selected in networks list screen."
    )
    public static let transactionTypeApprove = NSLocalizedString(
      "wallet.transactionTypeApprove",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approve",
      comment:
        "Explains that this transaction is an ERC20 approval transaction and is displayed among other transaction info"
    )
    public static let perGasPriceTitle = NSLocalizedString(
      "wallet.perGasPriceTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Per-gas price (Gwei)",
      comment: "A title above a text field for inputting the per-gas price limit in Gwei"
    )
    public static let editGasTitle = NSLocalizedString(
      "wallet.editGasTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Edit Gas",
      comment: "A title of the edit gas screen for standard transactions"
    )
    public static let lock = NSLocalizedString(
      "wallet.lock",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Lock",
      comment:
        "The title of the lock option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let lockWallet = NSLocalizedString(
      "wallet.lockWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Lock Wallet",
      comment:
        "The title of the lock option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let backUpWallet = NSLocalizedString(
      "wallet.backUpWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Back Up Now",
      comment:
        "The title of the back up wallet option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let settings = NSLocalizedString(
      "wallet.settings",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Settings",
      comment:
        "The title of the settings option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let walletSettings = NSLocalizedString(
      "wallet.walletSettings",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Wallet Settings",
      comment:
        "The title of the settings option inside the menu when user clicks the three dots button beside assets search button."
    )
    public static let balances = NSLocalizedString(
      "wallet.balances",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Balances",
      comment:
        "The title of the settings option inside the menu when user clicks the three dots button beside assets search button with Portfolio tab open."
    )
    public static let graph = NSLocalizedString(
      "wallet.graph",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Graph",
      comment:
        "The title of the settings option inside the menu when user clicks the three dots button beside assets search button with Portfolio tab open."
    )
    public static let nftsTab = NSLocalizedString(
      "wallet.nftsTab",
      tableName: "BraveWallet",
      bundle: .module,
      value: "NFTs Tab",
      comment:
        "The title of the settings option inside the menu when user clicks the three dots button beside assets search button with Portfolio tab open."
    )
    public static let helpCenter = NSLocalizedString(
      "wallet.helpCenter",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Help Center",
      comment:
        "The title of the Help Center option inside the menu when user clicks the three dots button beside assets search button or on wallet panel."
    )
    public static let swapDexAggrigatorNote = NSLocalizedString(
      "wallet.swapDexAggrigatorNote",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave uses %@ as a DEX aggregator.",
      comment:
        "A disclaimer note shown on the Swap screen. '%@' will be replaced by a company name, ex. '0x' / 'Jupiter'. 'DEX aggregator' is a type of blockchain-based service (decentralized exchange)"
    )
    public static let swapDexAggrigatorDisclaimer = NSLocalizedString(
      "wallet.swapDexAggrigatorDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "%@ will process the %@ address and IP address to fulfill a transaction (including getting quotes). %@ will ONLY use this data for the purposes of processing transactions.",
      comment:
        "A longer disclaimer about the DEX aggrigator used by Brave for swap transactions. The first '%@' is a company name. 'DEX aggregator' is a type of blockchain-based service (decentralized exchange). 'ONLY' is emphasized to show importance of the company's data usage."
    )
    public static let chartAxisDateLabel = NSLocalizedString(
      "wallet.chartAxisDateLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Date",
      comment:
        "The x-axis label on an asset or portfolio chart describing that x-axis values are defined by the date of the price"
    )
    public static let chartAxisPriceLabel = NSLocalizedString(
      "wallet.chartAxisPriceLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Price",
      comment:
        "The y-axis label on an asset or portfolio chart describing that y-axis values are based on the price at a given date"
    )
    public static let coinGeckoDisclaimer = NSLocalizedString(
      "wallet.coinGeckoDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Price data powered by CoinGecko",
      comment:
        "A disclaimer that appears at the bottom of an asset detail screen which shows prices and price history. CoinGecko is a third-party product."
    )
    public static let braveSwapFeeDisclaimer = NSLocalizedString(
      "wallet.braveSwapFeeDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Quote includes a %@ Brave fee.",
      comment:
        "A disclaimer that appears at the bottom of an swap screen which discloses the fixed Brave fee included in the swap quotes. '%@' will be replaced by a percentage. For example: 'Quote includes a 0.875% Brave fee'"
    )
    public static let screenshotDetectedTitle = NSLocalizedString(
      "wallet.screenshotDetectedTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Screenshot detected",
      comment: "A title of an alert when the user takes a screenshot of their device"
    )
    public static let recoveryPhraseScreenshotDetectedMessage = NSLocalizedString(
      "wallet.recoveryPhraseScreenshotDetectedMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Warning: A screenshot of your recovery phrase may get backed up to a cloud file service, and be readable by any application with photos access. Brave recommends that you not save this screenshot, and delete it as soon as possible.",
      comment: "The message displayed when the user takes a screenshot of their recovery phrase"
    )
    public static let privateKeyScreenshotDetectedMessage = NSLocalizedString(
      "wallet.privateKeyScreenshotDetectedMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Warning: A screenshot of your private key may get backed up to a cloud file service, and be readable by any application with photos access. Brave recommends that you not save this screenshot, and delete it as soon as possible.",
      comment: "The message displayed when the user takes a screenshot of their private key"
    )
    public static let sendWarningAddressIsOwn = NSLocalizedString(
      "wallet.sendWarningAddressIsOwn",
      tableName: "BraveWallet",
      bundle: .module,
      value: "The receiving address is your own address",
      comment:
        "A warning that appears below the send crypto address text field, when the input `To` address is the same as the current selected account's address."
    )
    public static let sendWarningAddressIsContract = NSLocalizedString(
      "wallet.sendWarningAddressIsContract",
      tableName: "BraveWallet",
      bundle: .module,
      value: "The receiving address is a token's contract address",
      comment:
        "A warning that appears below the send crypto address text field, when the input `To` address is a token contract address."
    )
    public static let sendWarningAddressNotValid = NSLocalizedString(
      "wallet.sendWarningAddressNotValid",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Not a valid ETH address",
      comment:
        "A warning that appears below the send crypto address text field, when the input `To` address is not a valid ETH address."
    )
    public static let sendWarningAddressMissingChecksumInfo = NSLocalizedString(
      "wallet.sendWarningAddressMissingChecksumInfo",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This address cannot be verified (missing checksum). Proceed?",
      comment:
        "A warning that appears below the send crypto address text field, when the input `To` address is missing checksum information."
    )
    public static let sendWarningAddressInvalidChecksum = NSLocalizedString(
      "wallet.sendWarningAddressInvalidChecksum",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Address did not pass verification (invalid checksum). Please try again, replacing lowercase letters with uppercase.",
      comment:
        "A warning that appears below the send crypto address text field, when the input `To` address has invalid checksum."
    )
    public static let sendWarningSolAddressNotValid = NSLocalizedString(
      "wallet.sendWarningSolAddressNotValid",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Not a valid SOL address",
      comment:
        "A warning that appears below the send crypto address text field, when the input `To` address is not a valid SOL address."
    )
    public static let sendErrorDomainNotRegistered = NSLocalizedString(
      "wallet.sendErrorDomainNotRegistered",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Domain doesn\'t have a linked %@ address",
      comment:
        "An error that appears below the send crypto address text field, when the input `To` domain/url that we cannot resolve to a wallet address. The '%@' will be replaced with the coin type Ex. `Domain doesn\'t have a linked ETH address`"
    )
    public static let sendErrorInvalidRecipientAddress = NSLocalizedString(
      "wallet.sendErrorInvalidRecipientAddress",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Invalid recipient address",
      comment:
        "An error that appears below the send crypto address text field, when the input `To` Filecoin address that is invalid"
    )
    public static let sendErrorBtcAddressNotValid = NSLocalizedString(
      "wallet.sendErrorBtcAddressNotValid",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Not a valid BTC address",
      comment:
        "An error that appears below the send crypto address text field, when the input `To` address is not a valid Bitcoin address."
    )
    public static let customNetworkChainIdTitle = NSLocalizedString(
      "wallet.customNetworkChainIdTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "The ID of the chain",
      comment:
        "The title above the input field for users to input network chain ID in Custom Network Details Screen."
    )
    public static let customNetworkChainIdPlaceholder = NSLocalizedString(
      "wallet.customNetworkChainIdPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter chain ID",
      comment:
        "The placeholder for the input field for users to input network chain ID in Custom Network Details Screen."
    )
    public static let customNetworkChainIdErrMsg = NSLocalizedString(
      "wallet.customNetworkChainIdErrMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Invalid format—chain ID must be a positive number.",
      comment:
        "The error message for the input field when users input an invalid value for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkChainNameTitle = NSLocalizedString(
      "wallet.customNetworkChainNameTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "The name of the chain",
      comment:
        "The title above the input field for users to input network chain name in Custom Network Details Screen."
    )
    public static let customNetworkChainNamePlaceholder = NSLocalizedString(
      "wallet.customNetworkChainNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter chain name",
      comment:
        "The placeholder for the input field for users to input network chain name in Custom Network Details Screen."
    )
    public static let customNetworkEmptyErrMsg = NSLocalizedString(
      "wallet.customNetworkEmptyErrMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This field cannot be blank.",
      comment:
        "The error message for the input fields cannot be empty in Custom Network Details Screen."
    )
    public static let customNetworkSymbolNameTitle = NSLocalizedString(
      "wallet.customNetworkSymbolNameTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Chain's currency name",
      comment:
        "The title above the input field for users to input network currency symbol name in Custom Network Details Screen."
    )
    public static let customNetworkSymbolNamePlaceholder = NSLocalizedString(
      "wallet.customNetworkSymbolNamePlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter currency name",
      comment:
        "The placeholder for the input field for users to input network currency symbol name in Custom Network Details Screen."
    )
    public static let customNetworkSymbolTitle = NSLocalizedString(
      "wallet.customNetworkSymbolTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Chain's currency symbol",
      comment:
        "The title above the input field for users to input network currency symbol in Custom Network Details Screen."
    )
    public static let customNetworkSymbolPlaceholder = NSLocalizedString(
      "wallet.customNetworkSymbolPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter currency symbol",
      comment:
        "The placeholder for the input field for users to input network currency symbol in Custom Network Details Screen."
    )
    public static let customNetworkCurrencyDecimalTitle = NSLocalizedString(
      "wallet.customNetworkCurrencyDecimalTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Chain's currency decimals",
      comment:
        "The title above the input field for users to input network currency decimal in Custom Network Details Screen."
    )
    public static let customNetworkCurrencyDecimalPlaceholder = NSLocalizedString(
      "wallet.customNetworkCurrencyDecimalPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter currency decimals",
      comment:
        "The placeholder for the input field for users to input network currency decimal in Custom Network Details Screen."
    )
    public static let customNetworkCurrencyDecimalErrMsg = NSLocalizedString(
      "wallet.customNetworkCurrencyDecimalErrMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Invalid format—currency decimals must be a positive number.",
      comment:
        "The error message for the input field when users input an invalid value for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkRpcUrlsTitle = NSLocalizedString(
      "wallet.customNetworkRpcUrlsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "RPC URLs",
      comment:
        "The title above the input fields for users to input network rpc urls in Custom Network Details Screen."
    )
    public static let customNetworkUrlsPlaceholder = NSLocalizedString(
      "wallet.customNetworkUrlsPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter URL(s)",
      comment:
        "The placeholder for the input field for users to input network rpc url in Custom Network Details Screen."
    )
    public static let customNetworkInvalidAddressErrMsg = NSLocalizedString(
      "wallet.customNetworkInvalidAddressErrMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Invalid address.",
      comment:
        "The error message for the input field when users input an invalid address for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkNotSecureErrMsg = NSLocalizedString(
      "wallet.customNetworkNotSecureErrMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Not secure.",
      comment:
        "The error message for the input field when users input an unsecure address for a custom network in Custom Network Details Screen."
    )
    public static let customNetworkIconUrlsTitle = NSLocalizedString(
      "wallet.customNetworkIconUrlsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Icon URLs",
      comment:
        "The title above the input fields for users to input network icon urls in Custom Network Details Screen."
    )
    public static let customNetworkBlockExplorerUrlsTitle = NSLocalizedString(
      "wallet.customNetworkBlockExplorerUrlsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Block explorer URLs",
      comment:
        "The title above the input fields for users to input network block explorer urls in Custom Network Details Screen."
    )
    public static let editfCustomNetworkTitle = NSLocalizedString(
      "wallet.editfCustomNetworkTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Edit Network",
      comment: "The title of form for users to edit an existed custom network."
    )
    public static let customNetworkDetailsTitle = NSLocalizedString(
      "wallet.customNetworkDetailsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add New Network",
      comment: "The title for Custom Network Details Screen for user to add or edit custom network."
    )
    public static let viewNetworkDetailsTitle = NSLocalizedString(
      "wallet.viewNetworkDetailsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Network Details",
      comment:
        "The title for View Network Details Screen shown when a user views the details of a network they are requested to add or switch to by a dapps website."
    )
    public static let settingsNetworkButtonTitle = NSLocalizedString(
      "wallet.settingsNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Networks",
      comment: "The title of a button that will go the list of networks."
    )
    public static let networkFooter = NSLocalizedString(
      "wallet.networkFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Wallet networks customization",
      comment: "The footer beneath the networks title in settings screen."
    )
    public static let networkIdDuplicationErrMsg = NSLocalizedString(
      "wallet.networkIdDuplicationErrMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Chain ID already exists",
      comment:
        "An error message will pop up when the user tries to add a custom network that its id already exists."
    )
    public static let failedToAddCustomNetworkErrorTitle = NSLocalizedString(
      "wallet.failedToAddCustomNetworkErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Failed to add this network.",
      comment:
        "The title of an alert when the custom network the user attempted to add fails for some reason"
    )
    public static let failedToRemoveCustomNetworkErrorMessage = NSLocalizedString(
      "wallet.failedToRemoveCustomNetworkErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Failed to remove network.\nPlease try again.",
      comment:
        "The message of an alert when the user attempted to remove custom network and it fails for some reason"
    )
    public static let addCustomNetworkBarItemTitle = NSLocalizedString(
      "wallet.addCustomNetworkBarItemTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add Network",
      comment: "The title of bar item for users to add custom network screen"
    )
    public static let addCustomNetworkDropdownButtonTitle = NSLocalizedString(
      "wallet.addCustomNetworkDropdownButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add Network…",
      comment:
        "The title of last option in the network selection dropdown menu. A short-cut for user to add new custom network."
    )
    public static let transactionBacklogTitle = NSLocalizedString(
      "wallet.transactionBacklogTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction backlog",
      comment: "Title of confirmation prompt when there's a backlog of wallet transactions"
    )
    public static let transactionBacklogBody = NSLocalizedString(
      "wallet.transactionBacklogBody",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "You're attempting to start a new transaction while you have previously submitted transactions that have not been confirmed. This will block any new ones from being submitted.",
      comment: "Text body of confirmation prompt when there's a backlog of wallet transactions"
    )
    public static let transactionBacklogAcknowledgement = NSLocalizedString(
      "wallet.transactionBacklogAcknowledgement",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Clear & replace the incomplete transaction(s)",
      comment:
        "Text of toggle for a confirmation prompt when there's a backlog of wallet transactions"
    )
    public static let learnMoreButton = NSLocalizedString(
      "wallet.learnMoreButton",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Learn More",
      comment:
        "Button to learn more about incomplete/pending wallet transactions. Or learn more about adding/switching networks in a dapp request view"
    )
    public static let transactionBacklogAfterReplacement = NSLocalizedString(
      "wallet.transactionBacklogAfterReplacement",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "For now, no additional action is required. Just wait for the unconfirmed transactions to clear.",
      comment:
        "Additional information, when users have previously clear and replace the incomplete transaction(s)"
    )
    public static let settingsResetTransactionTitle = NSLocalizedString(
      "wallet.settingsResetTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Clear Transaction & Nonce Info",
      comment:
        "The title of a button that will reset transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let settingsResetTransactionFooter = NSLocalizedString(
      "wallet.settingsResetTransactionFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Clearing transactions may be useful for developers or when clearing state on a local server",
      comment: "The footer message below the button to reset transaction"
    )
    public static let settingsResetTransactionAlertTitle = NSLocalizedString(
      "wallet.settingsResetTransactionAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Are you sure you want to reset transacton and nonce information?",
      comment:
        "The title the confirmation dialog when resetting transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let settingsResetTransactionAlertMessage = NSLocalizedString(
      "wallet.settingsResetTransactionAlertMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This option is mostly used by developers running a local test server",
      comment:
        "The message the confirmation dialog when resetting transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let settingsResetTransactionAlertButtonTitle = NSLocalizedString(
      "wallet.settingsResetTransactionAlertButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Reset",
      comment:
        "The title of a button that will reset transaction and nonce information. As in to erase the users transaction history and reset nonce value starting from 0x0"
    )
    public static let advancedSettingsTransaction = NSLocalizedString(
      "wallet.advancedSettingsTransaction",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Advanced settings",
      comment:
        "The title of the button that is displayed under the total gas fee in the confirmation transaction screen. Users can click it to go to advanced settings screen."
    )
    public static let editNonceHeader = NSLocalizedString(
      "wallet.editNonceHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Nonce",
      comment: "The header title for edit nonce section in advanced settings screen."
    )
    public static let editNonceFooter = NSLocalizedString(
      "wallet.editNonceFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Transaction may not be propagated to the network if custom nonce value is non-sequential or has other errors",
      comment: "The footer title for edit nonce section in advanced settings screen."
    )
    public static let editNoncePlaceholder = NSLocalizedString(
      "wallet.editNoncePlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enter custom nonce value",
      comment: "The placeholder for custom nonce textfield."
    )
    public static let editTransactionError = NSLocalizedString(
      "wallet.editTransactionError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Please check your inputs and try again.",
      comment:
        "The error alert body when something is wrong when users try to edit transaction gas fee, priority fee, nonce value or allowance value."
    )
    public static let editTransactionErrorCTA = NSLocalizedString(
      "wallet.editTransactionErrorCTA",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Go back",
      comment: "The transaction edit error alert button which will dismiss the alert."
    )
    public static let transactionDetailsTitle = NSLocalizedString(
      "wallet.transactionDetailsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Details",
      comment: "The title for the view displaying the details of a cryptocurrency transaction."
    )
    public static let transactionDetailsTxFeeTitle = NSLocalizedString(
      "wallet.transactionDetailsTxFeeTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction fee",
      comment:
        "The label for the fees involved in a cryptocurrency transaction. Appears next to the value transferred and the currency amount."
    )
    public static let transactionDetailsMarketPriceTitle = NSLocalizedString(
      "wallet.transactionDetailsMarketPriceTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Market price",
      comment:
        "The label for the market price of the asset used in a cryptocurrency transaction. Appears next to the formatted currency such as $1523.50"
    )
    public static let transactionDetailsDateTitle = NSLocalizedString(
      "wallet.transactionDetailsDateTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Date",
      comment:
        "The label for displaying the date a transaction occurred. Appears next to the formatted date such as '3:00PM - Jan 1 2022'"
    )
    public static let transactionDetailsNetworkTitle = NSLocalizedString(
      "wallet.transactionDetailsNetworkTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Network",
      comment:
        "The label for the network a transaction occurred on. Appears next to 'Ethereum Mainnet', 'Rinkeby Test Network', 'Ropsten Test Network', etc. "
    )
    public static let transactionDetailsTxHashTitle = NSLocalizedString(
      "wallet.transactionDetailsTransactionHashTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Hash",
      comment:
        "The label for the transaction hash (the identifier) of a cryptocurrency transaction. Appears next to a button that opens a URL for the transaction."
    )
    public static let transactionDetailsStatusTitle = NSLocalizedString(
      "wallet.transactionDetailsStatusTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Status",
      comment:
        "The label for the transaction status, which describes the how far along a transaction is to completing. Appears next to the words such as 'Approved', 'Submitted', 'Pending', etc."
    )
    public static let retryTransactionButtonTitle = NSLocalizedString(
      "wallet.retryTransactionButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Retry Transaction",
      comment:
        "The title for the button to retry a failed transaction."
    )
    public static let cancelTransactionButtonTitle = NSLocalizedString(
      "wallet.cancelTransactionButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Cancel Transaction",
      comment:
        "The title for the button to cancel a submitted transaction."
    )
    public static let speedUpTransactionButtonTitle = NSLocalizedString(
      "wallet.speedUpTransactionButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Speed Up",
      comment:
        "The title for the button to speed up a submitted transaction."
    )
    public static let errorAlertTitle = NSLocalizedString(
      "wallet.errorAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Error",
      comment: "A title displayed in an alert above the error message or description."
    )
    public static let sent = NSLocalizedString(
      "wallet.sent",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sent",
      comment: "As in sent cryptocurrency from one asset to another"
    )
    public static let settingsEnableBiometricsTitle = NSLocalizedString(
      "wallet.settingsEnableBiometricsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allow Biometric Unlock",
      comment:
        "A label beside the toggle for allowing biometrics to unlock the wallet in wallet settings."
    )
    public static let settingsEnableBiometricsFooter = NSLocalizedString(
      "wallet.settingsEnableBiometricsFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Toggle on if you'd like to use biometrics (Face ID / Touch ID) to unlock your wallet",
      comment:
        "The footer beneath the toggle for allowing biometrics to unlock the wallet in wallet settings."
    )
    public static let enterPasswordForBiometricsNavTitle = NSLocalizedString(
      "wallet.enterPasswordForBiometricsNavTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enable Biometrics",
      comment:
        "The navigation bar title displayed on the screen to enter wallet password to enable biometrics unlock from wallet settings."
    )
    public static let enterPasswordForBiometricsTitle = NSLocalizedString(
      "wallet.enterPasswordForBiometricsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Enter your wallet password to enable biometrics for wallet (you'll only be asked for this once)",
      comment:
        "The title displayed on the screen to enter password to enable biometrics unlock from wallet settings."
    )
    public static let dappsConnectionNotificationTitle = NSLocalizedString(
      "wallet.dappsConnectionNotificationTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This page wants to interact with Brave Wallet",
      comment:
        "The title of the notification which will prompt at the top of the browser when users are visiting web3 site that is not yet connected with Brave Wallet."
    )
    public static let dappsConnectionNotificationOriginTitle = NSLocalizedString(
      "wallet.dappsConnectionNotificationOriginTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ wants to interact with Brave Wallet",
      comment:
        "The title of the notification which will prompt at the top of the browser when users are visiting web3 site that is not yet connected with Brave Wallet. The '%@' will be the site attempting to connect. For example: \"app.uniswap.org wants to interact with Brave Wallet\""
    )
    public static let editPermissionsTitle = NSLocalizedString(
      "wallet.editPermissionsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Edit Permissions",
      comment:
        "The title of the detail view to edit the permissions / allowance of an ERC 20 Approve transaction."
    )
    public static let editPermissionsApproveUnlimited = NSLocalizedString(
      "wallet.editPermissionsApproveUnlimited",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unlimited",
      comment:
        "The value to show when the permissions / allowance of an ERC 20 Approve transaction are the maximum allowance or unlimited spending ammount."
    )
    public static let editPermissionsAllowanceHeader = NSLocalizedString(
      "wallet.editPermissionsAllowanceHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Spend limit permission allows %@ to withdraw and spend up to the following amount:",
      comment:
        "The header text shown above the rows to selected the allowance for an ERC 20 Approve transaction. The '%@' becomes the name of the account approving the transaction. For example: \"Spend limit permission allows Account 1 to withdraw and spend up to the following amount:\""
    )
    public static let editPermissionsProposedAllowanceHeader = NSLocalizedString(
      "wallet.editPermissionsProposedAllowanceHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Proposed allowance",
      comment: "The header text shown in the row that displays the proposed allowance value."
    )
    public static let editPermissionsCustomAllowanceHeader = NSLocalizedString(
      "wallet.editPermissionsCustomAllowanceHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Set custom allowance",
      comment: "The header text above the field to input a custom allowance value."
    )
    public static let editPermissionsSetUnlimited = NSLocalizedString(
      "wallet.editPermissionsSetUnlimited",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Set Unlimited",
      comment:
        "The title of the button shown beside the custom allowance input field to make the custom allowance value \"Unlimited\"."
    )
    public static let newSiteConnectScreenTitle = NSLocalizedString(
      "wallet.newSiteConnectScreenTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connect",
      comment: "The transaction edit error alert button which will dismiss the alert."
    )
    public static let newSiteConnectMessage = NSLocalizedString(
      "wallet.newSiteConnectMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connect to Brave Wallet",
      comment: "The message displayed below the dapp's origin url in new site connection screen."
    )
    public static let newSiteConnectFooter = NSLocalizedString(
      "wallet.newSiteConnectFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Only connect with sites you trust.",
      comment: "The footer message displayed above the Next button in new site connection screen."
    )
    public static let newSiteConnectConfirmationMessage = NSLocalizedString(
      "wallet.newSiteConnectConfirmationMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View the addresses of your permitted accounts (required)",
      comment:
        "A text displayed below the account address in new site connection confirmation step, in order to make sure users double check the account address they are going to allow the dapp to connect with."
    )
    public static let settingsDefaultBaseCurrencyTitle = NSLocalizedString(
      "wallet.settingsDefaultBaseCurrencyTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Default Base Currency",
      comment:
        "The title that appears before the current default base currency code. Example: \"Default base currency: USD\""
    )
    public static let addSuggestedTokenTitle = NSLocalizedString(
      "wallet.addSuggestedTokenTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add Suggested Token",
      comment:
        "The title of the view shown over a dapps website that requests the user add / approve a new token."
    )
    public static let addSuggestedTokenSubtitle = NSLocalizedString(
      "wallet.addSuggestedTokenSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Would you like to import this token?",
      comment:
        "The subtitle of the view shown over a dapps website that requests the user add / approve a new token, explaining to the user what this request does."
    )
    public static let contractAddressAccessibilityLabel = NSLocalizedString(
      "wallet.contractAddressAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Contract Address",
      comment: "The accessibility label for the contract address of a token / asset."
    )
    public static let switchNetworkTitle = NSLocalizedString(
      "wallet.switchNetworkTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Switch Network",
      comment:
        "The title of the view shown over a dapps website that requests the user change the current network."
    )
    public static let switchNetworkSubtitle = NSLocalizedString(
      "wallet.switchNetworkSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allow this site to switch the network?",
      comment:
        "The subtitle of the view shown over a dapps website that requests permission from the user to change the current network."
    )
    public static let switchNetworkDescription = NSLocalizedString(
      "wallet.switchNetworkDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This will switch the network to a previously added network",
      comment:
        "The description of the view shown over a dapps website that describes what switching the network will do."
    )
    public static let switchNetworkButtonTitle = NSLocalizedString(
      "wallet.switchNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Switch",
      comment:
        "The title of the button to approve switching the network on a switch network request from a dapps website."
    )
    public static let addNetworkTitle = NSLocalizedString(
      "wallet.addNetworkTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Add Network",
      comment:
        "The title of the view shown over a dapps website that requests the user add a new network."
    )
    public static let addNetworkSubtitle = NSLocalizedString(
      "wallet.addNetworkSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allow this site to add a network?",
      comment:
        "The subtitle of the view shown over a dapps website that requests permission from the user to add a new network."
    )
    public static let addNetworkDescription = NSLocalizedString(
      "wallet.addNetworkDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This will allow this network to be used within Brave Wallet.",
      comment:
        "The description of the view shown over a dapps website that describes what adding a new network will do."
    )
    public static let approveNetworkButtonTitle = NSLocalizedString(
      "wallet.approveNetworkButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approve",
      comment:
        "The title of the button to approve adding a new network on a add new network request from a dapps website."
    )
    public static let networkNameTitle = NSLocalizedString(
      "wallet.networkNameTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Network Name",
      comment:
        "The title for the label that is shown above the name of the network on a request to switch networks or add a new network from a dapps website."
    )
    public static let networkURLTitle = NSLocalizedString(
      "wallet.networkURLTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Network URL",
      comment:
        "The label that is shown above the URL for the network on a request to switch networks or add a new network from a dapps website."
    )
    public static let viewDetails = NSLocalizedString(
      "wallet.viewDetails",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View Details",
      comment:
        "The title for the button to view details about the network the dapp site is requesting the user switch to, or the network the dapp website is requesting the user add."
    )
    public static let signatureRequestTitle = NSLocalizedString(
      "wallet.signatureRequestTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Signature Requested",
      comment:
        "A title of the view shown over a dapps website that requests the user sign a message."
    )
    public static let signatureRequestSubtitle = NSLocalizedString(
      "wallet.signatureRequestSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Your signature is being requested",
      comment:
        "A subtitle of the view shown over a dapps website that requests the user sign a message."
    )
    public static let signatureRequestDomainTitle = NSLocalizedString(
      "wallet.signatureRequestDomainTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Domain:",
      comment:
        "A title displayed inside the text view in Signature Request View above the request's domain information."
    )
    public static let signatureRequestMessageTitle = NSLocalizedString(
      "wallet.signatureRequestMessageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Message:",
      comment:
        "A title displayed inside the text view in Signature Request View above the request's message."
    )
    public static let sign = NSLocalizedString(
      "wallet.sign",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign",
      comment:
        "The title of the button used to sign a message request on the signature request view."
    )
    public static let web3PreferencesSectionTitle = NSLocalizedString(
      "wallet.web3PreferencesSectionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Web3 preferences",
      comment: "The section title for users to set up preferences for interation with web3 sites."
    )
    public static let web3PreferencesDefaultEthWallet = NSLocalizedString(
      "wallet.web3PreferencesDefaultEthWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Default Ethereum Wallet",
      comment:
        "The title for the entry displaying the current preferred default Ethereum wallet is."
    )
    public static let web3PreferencesDefaultSolWallet = NSLocalizedString(
      "wallet.web3PreferencesDefaultSolWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Default Solana Wallet",
      comment: "The title for the entry displaying the current preferred default Solana wallet is."
    )
    public static let web3PreferencesAllowEthProviderAccess = NSLocalizedString(
      "wallet.web3PreferencesAllowEthProviderAccess",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sites Can Request Access to Your Ethereum Wallet",
      comment:
        "The title for the entry displaying the preferred option to allow web3 sites to access the Ethereum provider API."
    )
    public static let web3PreferencesAllowSolProviderAccess = NSLocalizedString(
      "wallet.web3PreferencesAllowSolProviderAccess",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sites Can Request Access to Your Solana Wallet",
      comment:
        "The title for the entry displaying the preferred option to allow web3 sites to access the Solana provider API."
    )
    public static let web3PreferencesDisplayWeb3Notifications = NSLocalizedString(
      "wallet.web3PreferencesDisplayWeb3Notifications",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Display Web3 Notifications",
      comment:
        "The title for the entry displaying the preferred option to display web3 site notifications."
    )
    public static let web3PreferencesManageSiteConnections = NSLocalizedString(
      "wallet.web3PreferencesManageSiteConnections",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Manage Site Connections",
      comment:
        "The title for the entry that by clicking will direct users to the screen to manage web3 sites account connections, and the title for the Manage Site Connections screen."
    )
    public static let manageSiteConnectionsFilterPlaceholder = NSLocalizedString(
      "wallet.manageSiteConnectionsFilterPlaceholder",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Filter Connected DApps",
      comment:
        "The filter in the search bar for the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsRemoveAll = NSLocalizedString(
      "wallet.manageSiteConnectionsRemoveAll",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Remove All",
      comment:
        "The title of the button on the screen to manage web3 sites account connections that will show an alert to remove all connected website permissions."
    )
    public static let manageSiteConnectionsConfirmAlertTitle = NSLocalizedString(
      "wallet.manageSiteConnectionsConfirmAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Remove all permissions?",
      comment:
        "The title of the alert to confirm the users wants to remove all site connections, shown on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsConfirmAlertMessage = NSLocalizedString(
      "wallet.manageSiteConnectionsConfirmAlertMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This will remove all Wallet connection permissions for all websites.",
      comment:
        "The message of the alert to confirm the users wants to remove all site connections, shown on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsDetailConfirmAlertMessage = NSLocalizedString(
      "wallet.manageSiteConnectionsDetailConfirmAlertMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This will remove all Wallet connection permissions for this website.",
      comment:
        "The message of the alert to confirm the users wants to remove all site connections from this specific website, shown after selecting/opening a website on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsConfirmAlertRemove = NSLocalizedString(
      "wallet.manageSiteConnectionsConfirmAlertRemove",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Remove",
      comment:
        "The title of the confirmation button in the alert to confirm the users wants to remove all site connections, shown on the screen to manage web3 sites account connections."
    )
    public static let manageSiteConnectionsAccount = NSLocalizedString(
      "wallet.manageSiteConnectionsAccount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%lld %@",
      comment:
        "The amount of current permitted wallet accounts to the dapp site. It is displayed below the origin url of the dapp site in manage site connections screen. '%lld' refers to a number, %@ is `account` for singular and `accounts` for plural (for example \"1 account\" or \"2 accounts\")"
    )
    public static let manageSiteConnectionsAccountSingular = NSLocalizedString(
      "wallet.manageSiteConnectionsAccountSingular",
      tableName: "BraveWallet",
      bundle: .module,
      value: "account",
      comment: "The singular word that will be used in `manageSiteConnectionsAccount`."
    )
    public static let manageSiteConnectionsAccountPlural = NSLocalizedString(
      "wallet.manageSiteConnectionsAccountPlural",
      tableName: "BraveWallet",
      bundle: .module,
      value: "accounts",
      comment: "The plural word that will beused in `manageSiteConnectionsAccount`."
    )
    public static let manageSiteConnectionsDetailHeader = NSLocalizedString(
      "wallet.manageSiteConnectionsDetailHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connected %@ Accounts",
      comment:
        "The header shown above the list of connected accounts for a single website, shown after selecting/opening a website on the screen to manage web3 sites account connections. '%@' will be replaced by a coin type title. For example, it could be 'Connected Ethereum Accounts' or 'Connected Solana Accounts' etc."
    )
    public static let walletTypeNone = NSLocalizedString(
      "wallet.walletTypeNone",
      tableName: "BraveWallet",
      bundle: .module,
      value: "None",
      comment:
        "The value shown when selecting the default wallet as none / no wallet in wallet settings, or when grouping Portfolio assets."
    )
    public static let unlockWallet = NSLocalizedString(
      "wallet.unlockWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unlock Wallet",
      comment:
        "The title of the button in wallet panel when wallet is locked. Users can click it to open full screen unlock wallet screen."
    )
    public static let walletPanelSetupWalletDescription = NSLocalizedString(
      "wallet.walletPanelSetupWalletDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Use this panel to securely access web3 and all your crypto assets.",
      comment: "The description for wallet panel when users haven't set up a wallet yet."
    )
    public static let walletFullScreenAccessibilityTitle = NSLocalizedString(
      "wallet.walletFullScreenAccessibilityTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Open wallet in full screen",
      comment:
        "The label read out when a user is using VoiceOver and highlights the two-arrow button on the wallet panel top left corner."
    )
    public static let editSiteConnectionScreenTitle = NSLocalizedString(
      "wallet.editSiteConnectionScreenTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connections",
      comment:
        "The navigation title of the screen for users to edit dapps site connection with users' Brave Wallet accounts."
    )
    public static let editSiteConnectionConnectedAccount = NSLocalizedString(
      "wallet.editSiteConnectionConnectedAccount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%lld %@ connected",
      comment:
        "The amount of current permitted wallet accounts to the dapp site. It is displayed below the origin url of the dapp site in edit site connection screen. '%lld' refers to a number, %@ is `account` for singular and `accounts` for plural (for example \"1 account connected\" or \"2 accounts connected\")"
    )
    public static let editSiteConnectionAccountSingular = NSLocalizedString(
      "wallet.editSiteConnectionAccountSingular",
      tableName: "BraveWallet",
      bundle: .module,
      value: "account",
      comment: "The singular word that will be used in `editSiteConnectionConnectedAccount`."
    )
    public static let editSiteConnectionAccountPlural = NSLocalizedString(
      "wallet.editSiteConnectionAccountPlural",
      tableName: "BraveWallet",
      bundle: .module,
      value: "accounts",
      comment: "The plural word that will beused in `editSiteConnectionConnectedAccount`."
    )
    public static let editSiteConnectionAccountActionConnect = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionConnect",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connect",
      comment:
        "The title of the button for users to click so that they connect wallet account to the dapp site(also permission given). It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let editSiteConnectionAccountActionDisconnect = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionDisconnect",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Disconnect",
      comment:
        "The title of the button for users to click so that they disconnect wallet account to the dapp site(also permission removed). It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let editSiteConnectionAccountActionSwitch = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionSwitch",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Switch",
      comment:
        "The title of the button for users to click so that they disconnect wallet account to the dapp site(also permission removed). It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let editSiteConnectionAccountActionTrust = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionTrust",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Trust",
      comment:
        "The title of the button for users to click so that they grant wallet account permission for the Solana Dapp. It will be displayed at the right hand side of each account option in edit site connection screen."
    )
    public static let editSiteConnectionAccountActionRevoke = NSLocalizedString(
      "wallet.editSiteConnectionAccountActionRevoke",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Revoke",
      comment:
        "The title of the button for users to click so that they remove wallet account permission from the Solana Dapp. It will be displayed at the right hand side of each account option in edit site connection screen"
    )
    public static let walletPanelConnected = NSLocalizedString(
      "wallet.walletPanelConnected",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connected",
      comment:
        "The title of the button for users to click to go to edit site connection screen. This title indicates the user is currently connected his/her wallet account to the dapp. The title will be displayed to the right of a checkmark."
    )
    public static let walletPanelConnect = NSLocalizedString(
      "wallet.walletPanelConnect",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connect",
      comment:
        "The title of the button for users to click to go to edit site connection screen. This title indicates the user is currently not connected any his/her wallet account to the dapp."
    )
    public static let walletPanelDisconnected = NSLocalizedString(
      "wallet.walletPanelDisconnected",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Disconnected",
      comment:
        "The title of the button for users to click to go to edit site connection screen. This title indicates the user is currently not connected his/her current selected Solana account to the Solana Dapp."
    )
    public static let walletPanelBlocked = NSLocalizedString(
      "wallet.walletPanelBlocked",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Blocked",
      comment:
        "The title of the button for users to click to go to edit site connection screen. This title indicates the user is currently blocking all the accesses to the solana provider APIs."
    )
    public static let getEncryptionPublicKeyRequestTitle = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Public Encryption Key Request",
      comment:
        "A title of the view shown over a dapps website that requests the users public encryption key."
    )
    public static let getEncryptionPublicKeyRequestSubtitle = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "A DApp is requesting your public encryption key",
      comment:
        "A subtitle of the view shown over a dapps website that requests the users public encryption key."
    )
    public static let getEncryptionPublicKeyRequestMessage = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "is requesting your wallet's public encryption key. If you consent to providing this key, the site will be able to compose encrypted messages to you.",
      comment:
        "The text shown beside the URL origin of a get public encryption key request from a dapp site. Ex 'https://brave.com is requesting your wallets public encryption key. If you consent to providing this key, the site will be able to compose encrypted messages to you.'"
    )
    public static let getEncryptionPublicKeyRequestApprove = NSLocalizedString(
      "wallet.getEncryptionPublicKeyRequestApprove",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Provide",
      comment:
        "The title of the button to approve a encryption public key request from a dapps website."
    )
    public static let decryptRequestTitle = NSLocalizedString(
      "wallet.decryptRequestTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Decrypt Request",
      comment:
        "A title of the view shown over a dapps website that requests the user decrypt a message."
    )
    public static let decryptRequestSubtitle = NSLocalizedString(
      "wallet.decryptRequestSubtitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This DApp would like to read this message to complete your request",
      comment:
        "A subtitle of the view shown over a dapps website that requests the user decrypt a message."
    )
    public static let decryptRequestApprove = NSLocalizedString(
      "wallet.decryptRequestApprove",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allow",
      comment: "The title of the button to approve a decrypt request from a dapps website."
    )
    public static let decryptRequestReveal = NSLocalizedString(
      "wallet.decryptRequestReveal",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Reveal",
      comment:
        "The title of the button to show the message of a decrypt request from a dapps website."
    )
    public static let decryptMessageScreenshotDetectedMessage = NSLocalizedString(
      "wallet.decryptMessageScreenshotDetectedMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Warning: A screenshot of your message may get backed up to a cloud file service, and be readable by any application with photos access. Brave recommends that you not save this screenshot, and delete it as soon as possible.",
      comment:
        "The message displayed when the user takes a screenshot of their dapp decrypt request."
    )
    public static let coinTypeEthereum = NSLocalizedString(
      "wallet.coinTypeEthereum",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Ethereum",
      comment:
        "One of the coin types for users to create an account in Ethereum mainnet or any EVM networks."
    )
    public static let coinTypeSolana = NSLocalizedString(
      "wallet.coinTypeSolana",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Solana",
      comment: "One of the coin types for users to create an account with in Solana network"
    )
    public static let coinTypeFilecoin = NSLocalizedString(
      "wallet.coinTypeFilecoin",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Filecoin",
      comment: "One of the coin types for users to create an account to store FIL assets"
    )
    public static let coinTypeBitcoin = NSLocalizedString(
      "wallet.coinTypeBitcoin",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Bitcoin",
      comment: "One of the coin types for users to create an account to store BTC assets"
    )
    public static let coinTypeEthereumDescription = NSLocalizedString(
      "wallet.coinTypeEthereumDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Supports EVM compatible assets on the Ethereum blockchain (ERC-20, ERC-721, ERC-1551, ERC-1155)",
      comment: "A description for Ethereum coin type."
    )
    public static let coinTypeSolanaDescription = NSLocalizedString(
      "wallet.coinTypeSolanaDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Supports SPL compatible assets on the Solana blockchain",
      comment: "A description for Solana coin type."
    )
    public static let coinTypeFilecoinDescription = NSLocalizedString(
      "wallet.coinTypeFilecoinDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Store FIL asset",
      comment: "A description for Filecoin coin type."
    )
    public static let coinTypeBitcoinDescription = NSLocalizedString(
      "wallet.coinTypeBitcoinDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Store BTC asset",
      comment: "A description for Bitcoin coin type."
    )
    public static let coinTypeUnknown = NSLocalizedString(
      "wallet.coinTypeUnknown",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unknown",
      comment: "A placeholder for unknown coin type."
    )
    public static let coinTypeSelectionHeader = NSLocalizedString(
      "wallet.coinTypeSelectionHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select one of the following account types",
      comment: "A header displayed above coin types selection."
    )
    public static let createAccountAlertTitle = NSLocalizedString(
      "wallet.createAccountAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You don't have a %@ account",
      comment:
        "The title of the alert shown when a user switches to a network they do not have an account for yet. '%@' will be replaced with a network name such as 'Solana' or 'Filecoin'."
    )
    public static let createAccountAlertMessage = NSLocalizedString(
      "wallet.createAccountAlertMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Create one now?",
      comment:
        "The message of the alert shown when a user switches to a network they do not have an account for yet."
    )
    public static let networkSelectionTitle = NSLocalizedString(
      "wallet.networkSelectionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select Network",
      comment: "The title of the to select a network from the available networks"
    )
    public static let networkSelectionPrimaryNetworks = NSLocalizedString(
      "wallet.networkSelectionPrimaryNetworks",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Primary Networks",
      comment: "The title of the section for primary networks in the network selection view."
    )
    public static let networkSelectionSecondaryNetworks = NSLocalizedString(
      "wallet.networkSelectionSecondaryNetworks",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Secondary Networks",
      comment: "The title of the section for secondary networks in the network selection view."
    )
    public static let networkSelectionTestNetworks = NSLocalizedString(
      "wallet.networkSelectionTestNetworks",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Test Networks",
      comment: "The title of the section for test networks in the network selection view."
    )
    public static let networkSelectionTestnetAccessibilityLabel = NSLocalizedString(
      "wallet.networkSelectionTestnetAccessibilityLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View %@ test networks",
      comment:
        "A VoiceOver label that will be read out when a user focuses on the show test networks button in the network selection view. \"%@\" will be replaced with the network name such as \"Solana\" or \"Ethereum\""
    )
    public static let networkNotSupportedForBuyToken = NSLocalizedString(
      "wallet.networkNotSupportedForBuyToken",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Buy not supported for selected network",
      comment:
        "A placeholder in Buy Screen, when user switched to a network that Brave currently doesn't support token purchasing."
    )
    public static let auroraBridgeAlertTitle = NSLocalizedString(
      "wallet.auroraBridgeAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Open the Rainbow Bridge App?",
      comment:
        "The title of the alert that will show up when users click on `Bridge to Aurora` button inside asset details screen for tokens that support briding to `Aurora`."
    )
    public static let auroraBridgeAlertDescription = NSLocalizedString(
      "wallet.auroraBridgeAlertDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Rainbow Bridge is an independent service that helps you bridge assets across networks, and use your crypto on other networks and DApp ecosystems. Bridging assets to other networks has some risks.",
      comment:
        "The description of the alert that will show up when users click on `Bridge to Aurora` button inside asset details screen for tokens that support briding to `Aurora`. This will displayed right below `auroraBridgeAlertTitle`."
    )
    public static let auroraBridgeButtonTitle = NSLocalizedString(
      "wallet.auroraBridgeButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Bridge to Aurora",
      comment:
        "The title for the button for users to click inside any asset details screen, whose asset is supported to redirect users to Aurora site."
    )
    public static let auroraBridgeLearnMore = NSLocalizedString(
      "wallet.auroraBridgeLearnMore",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Learn more about using Rainbow Bridge",
      comment:
        "A text link for user to click inside the custom alert, It will redirect user to the website explaining using Rainbow Bridge."
    )
    public static let auroraBridgeRisk = NSLocalizedString(
      "wallet.auroraBridgeRisk",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Learn how to reduce risk on Rainbow Bridge",
      comment:
        "A text link for user to click inside the custom alert, It will redirect user to the website explaining the risk of using Rainbow Bridge."
    )
    public static let auroraPopupDontShowAgain = NSLocalizedString(
      "wallet.auroraPopupDontShowAgain",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Don't show again",
      comment: "A text button for user to click so this pop up will not show again."
    )
    public static let dappsSettingsNavTitle = NSLocalizedString(
      "wallet.dappsSettingsNavTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ Settings",
      comment:
        "The navigation title of the settings screen for each Dapp. '%@' will be relpaced by the name of the coin type. For example, the title could 'Ethereum Settings', 'Solana Settings' etc."
    )
    public static let dappsSettingsRemoveAllWarning = NSLocalizedString(
      "wallet.dappsSettingsRemoveAllWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This will remove all Wallet connection permissions for %d %@",
      comment:
        "The warning message in a confirmation dialog when attempting to remove all wallet connection to a dapp."
    )
    public static let dappsSettingsWebsiteSingular = NSLocalizedString(
      "wallet.dappsSettingsWebsiteSingular",
      tableName: "BraveWallet",
      bundle: .module,
      value: "website",
      comment: "The singular word that will be used in `manageSiteConnectionsAccount`."
    )
    public static let dappsSettingsWebsitePlural = NSLocalizedString(
      "wallet.dappsSettingsWebsitePlural",
      tableName: "BraveWallet",
      bundle: .module,
      value: "websites",
      comment: "The plural word that will be used in `manageSiteConnectionsAccount`."
    )
    public static let dappsSettingsGeneralSectionTitle = NSLocalizedString(
      "wallet.dappsSettingsGeneralSectionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "General",
      comment:
        "The title of the section that will display settings for default wallet and dapp provider api access permission"
    )
    public static let dappsSettingsConnectedSitesSectionTitle = NSLocalizedString(
      "wallet.dappsSettingsConnectedSitesSectionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connected DApps",
      comment: "The title of the section that will list all the permitted dapp connections"
    )
    public static let dappsSettingsConnectedSitesSectionEmpty = NSLocalizedString(
      "wallet.dappsSettingsConnectedSitesSectionEmpty",
      tableName: "BraveWallet",
      bundle: .module,
      value: "DApps you connect to Brave Wallet will appear here",
      comment:
        "A message that will be displayed under the section header when there is no dapps have been granted wallet connection."
    )
    public static let signTransactionTitle = NSLocalizedString(
      "wallet.signTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign Transaction",
      comment:
        "The title of the view shown over a dapps website that requests the user sign a transaction."
    )
    public static let signAllTransactionsTitle = NSLocalizedString(
      "wallet.signAllTransactionsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign All Transactions",
      comment:
        "The title of the view shown over a dapps website that requests the user sign all displayed transactions."
    )
    public static let confirmPasswordTitle = NSLocalizedString(
      "wallet.confirmPasswordTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Confirm Password",
      comment:
        "The title displayed in the navigation of the confirmation view where a user tries to remove a secondary account."
    )
    public static let removeAccountConfirmationMessage = NSLocalizedString(
      "wallet.removeAccountConfirmationMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Are you sure you want to remove \"%@\"?",
      comment:
        "A message that will be displayed above the password entry field when the user tries to remove a secondary account. '%@' will be replaced with the account's name"
    )
    public static let removeAccountErrorMessage = NSLocalizedString(
      "wallet.removeAccountErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Invalid Password",
      comment:
        "A message that will be displayed under the password entry field when the user enters an incorrect password and their secondary account is not removed."
    )
    public static let providerSelectionScreenTitle = NSLocalizedString(
      "wallet.providerSelectionScreenTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select purchase method",
      comment:
        "The title of the screen for users to select a purchase method. For example, 'Ramp.Network'."
    )
    public static let providerSelectionSectionHeader = NSLocalizedString(
      "wallet.providerSelectionSectionHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select one of the following options",
      comment: "The section header for the list of available purchse methods."
    )
    public static let providerSelectionButtonTitle = NSLocalizedString(
      "wallet.providerSelectionButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Buy with %@",
      comment:
        "The button title for users to click. It will redirect user to that specific provider website. '%@' will be replaced provider's short name. For example, Buy with Ramp'."
    )
    public static let rampNetworkProviderName = NSLocalizedString(
      "wallet.rampNetworkProviderName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Ramp.Network",
      comment: "The name of one of the on ramp provider."
    )
    public static let rampNetworkProviderShortName = NSLocalizedString(
      "wallet.rampNetworkProviderShortName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Ramp",
      comment: "The short name of one of the on ramp provider."
    )
    public static let rampNetworkProviderDescription = NSLocalizedString(
      "wallet.rampNetworkProviderDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Buy crypto with Visa or Mastercard.",
      comment: "The description of one of the 'Ramp.Network' provider."
    )
    public static let sardineProviderName = NSLocalizedString(
      "wallet.sardineProviderName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sardine",
      comment: "The name of one of the on ramp provider."
    )
    public static let sardineProviderShortName = NSLocalizedString(
      "wallet.sardineProviderShortName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sardine",
      comment: "The short name of one of the on ramp provider."
    )
    public static let sardineProviderDescription = NSLocalizedString(
      "wallet.sardineProviderDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Instant buy with your bank account. Lower fees.",
      comment: "The description of one of the 'Sardine' provider."
    )
    public static let transakProviderName = NSLocalizedString(
      "wallet.transakProviderName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transak",
      comment: "The name of one of the on ramp provider."
    )
    public static let transakProviderShortName = NSLocalizedString(
      "wallet.transakProviderShortName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transak",
      comment: "The short name of one of the on ramp provider."
    )
    public static let transakProviderDescription = NSLocalizedString(
      "wallet.transakProviderDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Instant buy with your bank account. Lower fees.",
      comment: "The description of one of the 'Transak' provider."
    )
    public static let stripeNetworkProviderName = NSLocalizedString(
      "wallet.stripeNetworkProviderName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ by %@",
      comment:
        "The name of one of the on ramp providers where the first '%@' is `Link` and second '%@' is `Stripe` product names."
    )
    public static let stripeNetworkProviderDescription = NSLocalizedString(
      "wallet.stripeNetworkProviderDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Pay with credit, debit, bank account.",
      comment: "The description of one of the 'Stripe' provider."
    )
    public static let coinbaseNetworkProviderDescription = NSLocalizedString(
      "wallet.coinbaseNetworkProviderDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Buy with the most trusted name in crypto.",
      comment: "The description of one of the 'Coinbase Pay' provider."
    )
    public static let solanaDappTransactionTitle = NSLocalizedString(
      "wallet.solanaDappTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approve Transaction",
      comment:
        "The title displayed above the value of a Solana dapp transaction in transaction confirmation view, transaction details view and transaction summary rows."
    )
    public static let solanaSwapTransactionTitle = NSLocalizedString(
      "wallet.solanaSwapTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap Transaction",
      comment:
        "The title displayed above the value of a Solana swap transaction in transaction confirmation view, transaction details view and transaction summary rows."
    )
    public static let solanaSystemProgramName = NSLocalizedString(
      "wallet.solanaSystemProgramName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "System Program - %@",
      comment:
        "The title displayed beside the name of any Solana System Program instruction type. '%@' will be replaced with the instruction's name, ex. \"System Program - Transfer\""
    )
    public static let solanaTransferInstructionName = NSLocalizedString(
      "wallet.solanaTransferInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transfer",
      comment:
        "The title displayed above the System Program & Token Program Transfer instruction type for Solana Instruction details."
    )
    public static let solanaTransferWithSeedInstructionName = NSLocalizedString(
      "wallet.solanaTransferWithSeedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transfer With Seed",
      comment:
        "The title displayed above the System Program TransferWithSeed instruction type for Solana Instruction details."
    )
    public static let solanaWithdrawNonceAccountInstructionName = NSLocalizedString(
      "wallet.solanaWithdrawNonceAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Withdraw From Nonce Account",
      comment:
        "The title displayed above the System Program WithdrawNonceAccount instruction type for Solana Instruction details."
    )
    public static let solanaCreateAccountInstructionName = NSLocalizedString(
      "wallet.solanaCreateAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Create Account",
      comment:
        "The title displayed above the System Program CreateAccount instruction type for Solana Instruction details."
    )
    public static let solanaCreateAccountWithSeedInstructionName = NSLocalizedString(
      "wallet.solanaCreateAccountWithSeedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Create Account With Seed",
      comment:
        "The title displayed above the System Program CreateAccountWithSeed instruction type for Solana Instruction details."
    )
    public static let solanaAssignInstructionName = NSLocalizedString(
      "wallet.solanaAssignInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Assign",
      comment:
        "The title displayed above the System Program Assign instruction type for Solana Instruction details."
    )
    public static let solanaAssignWithSeedInstructionName = NSLocalizedString(
      "wallet.solanaAssignWithSeedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Assign With Seed",
      comment:
        "The title displayed above the System Program AssignWithSeed instruction type for Solana Instruction details."
    )
    public static let solanaAllocateInstructionName = NSLocalizedString(
      "wallet.solanaAllocateInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allocate",
      comment:
        "The title displayed above the System Program Allocate instruction type for Solana Instruction details."
    )
    public static let solanaAllocateWithSeedInstructionName = NSLocalizedString(
      "wallet.solanaAllocateWithSeedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allocate With Seed",
      comment:
        "The title displayed above the System Program AllocateWithSeed instruction type for Solana Instruction details."
    )
    public static let solanaAdvanceNonceAccountInstructionName = NSLocalizedString(
      "wallet.solanaAdvanceNonceAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Advance Nonce Account",
      comment:
        "The title displayed above the System Program AdvanceNonceAccount instruction type for Solana Instruction details."
    )
    public static let solanaInitializeNonceAccountInstructionName = NSLocalizedString(
      "wallet.solanaInitializeNonceAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Nonce Account",
      comment:
        "The title displayed above the System Program InitializeNonceAccount instruction type for Solana Instruction details."
    )
    public static let solanaAuthorizeNonceAccountInstructionName = NSLocalizedString(
      "wallet.solanaAuthorizeNonceAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Authorize Nonce Account",
      comment:
        "The title displayed above the System Program AuthorizeNonceAccount instruction type for Solana Instruction details."
    )
    public static let solanaUpgradeNonceAccountInstructionName = NSLocalizedString(
      "wallet.solanaUpgradeNonceAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Upgrade Nonce Account",
      comment:
        "The title displayed above the System Program UpgradeNonceAccount instruction type for Solana Instruction details."
    )
    public static let solanaTokenProgramName = NSLocalizedString(
      "wallet.solanaTokenProgramName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token Program - %@",
      comment:
        "The title displayed beside the name of any Solana Token Program instruction type. '%@' will be replaced with the instruction's name, ex. \"Token Program - Initialize Mint\""
    )
    public static let solanaInitializeMintInstructionName = NSLocalizedString(
      "wallet.solanaInitializeMintInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Mint",
      comment:
        "The title displayed above the Token Program InitializeMint instruction type for Solana Instruction details."
    )
    public static let solanaInitializeMint2InstructionName = NSLocalizedString(
      "wallet.solanaInitializeMint2InstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Mint 2",
      comment:
        "The title displayed above the Token Program InitializeMint2 instruction type for Solana Instruction details."
    )
    public static let solanaInitializeAccountInstructionName = NSLocalizedString(
      "wallet.solanaInitializeAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Account",
      comment:
        "The title displayed above the Token Program InitializeAccount instruction type for Solana Instruction details."
    )
    public static let solanaInitializeAccount2InstructionName = NSLocalizedString(
      "wallet.solanaInitializeAccount2InstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Account 2",
      comment:
        "The title displayed above the Token Program InitializeAccount2 instruction type for Solana Instruction details."
    )
    public static let solanaInitializeAccount3InstructionName = NSLocalizedString(
      "wallet.solanaInitializeAccount3InstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Account 3",
      comment:
        "The title displayed above the Token Program InitializeAccount3 instruction type for Solana Instruction details."
    )
    public static let solanaInitializeMultisigInstructionName = NSLocalizedString(
      "wallet.solanaInitializeMultisigInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Multisig",
      comment:
        "The title displayed above the Token Program InitializeMultisig instruction type for Solana Instruction details."
    )
    public static let solanaInitializeMultisig2InstructionName = NSLocalizedString(
      "wallet.solanaInitializeMultisig2InstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Initialize Multisig 2",
      comment:
        "The title displayed above the Token Program InitializeMultisig2 instruction type for Solana Instruction details."
    )
    public static let solanaApproveInstructionName = NSLocalizedString(
      "wallet.solanaApproveInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approve",
      comment:
        "The title displayed above the Token Program Approve instruction type for Solana Instruction details."
    )
    public static let solanaRevokeInstructionName = NSLocalizedString(
      "wallet.solanaRevokeInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Revoke",
      comment:
        "The title displayed above the Token Program Revoke instruction type for Solana Instruction details."
    )
    public static let solanaSetAuthorityInstructionName = NSLocalizedString(
      "wallet.solanaSetAuthorityInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Set Authority",
      comment:
        "The title displayed above the Token Program Set Authority instruction type for Solana Instruction details."
    )
    public static let solanaMintToInstructionName = NSLocalizedString(
      "wallet.solanaMintToInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Mint To",
      comment:
        "The title displayed above the Token Program Mint To instruction type for Solana Instruction details."
    )
    public static let solanaBurnInstructionName = NSLocalizedString(
      "wallet.solanaBurnInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Burn",
      comment:
        "The title displayed above the Token Program Burn instruction type for Solana Instruction details."
    )
    public static let solanaCloseAccountInstructionName = NSLocalizedString(
      "wallet.solanaCloseAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Close Account",
      comment:
        "The title displayed above the Token Program Close Account instruction type for Solana Instruction details."
    )
    public static let solanaFreezeAccountInstructionName = NSLocalizedString(
      "wallet.solanaFreezeAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Freeze Account",
      comment:
        "The title displayed above the Token Program Freeze Account instruction type for Solana Instruction details."
    )
    public static let solanaThawAccountInstructionName = NSLocalizedString(
      "wallet.solanaThawAccountInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Thaw Account",
      comment:
        "The title displayed above the Token Program Thaw Account instruction type for Solana Instruction details."
    )
    public static let solanaApproveCheckedInstructionName = NSLocalizedString(
      "wallet.solanaApproveCheckedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approve Checked",
      comment:
        "The title displayed above the Token Program Approved Checked instruction type for Solana Instruction details."
    )
    public static let solanaTransferCheckedInstructionName = NSLocalizedString(
      "wallet.solanaTransferCheckedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transfer Checked",
      comment:
        "The title displayed above the Token Program Transfer Checked instruction type for Solana Instruction details."
    )
    public static let solanaMintToCheckedInstructionName = NSLocalizedString(
      "wallet.solanaMintToCheckedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Mint To Checked",
      comment:
        "The title displayed above the Token Program Mint To Checked instruction type for Solana Instruction details."
    )
    public static let solanaBurnCheckedInstructionName = NSLocalizedString(
      "wallet.solanaBurnCheckedInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Burn Checked",
      comment:
        "The title displayed above the Token Program Burn Checked instruction type for Solana Instruction details."
    )
    public static let solanaSyncNativeInstructionName = NSLocalizedString(
      "wallet.solanaSyncNativeInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sync Native",
      comment:
        "The title displayed above the Token Program Sync Native instruction type for Solana Instruction details."
    )
    public static let solanaAmount = NSLocalizedString(
      "wallet.solanaAmount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Amount",
      comment:
        "The label displayed beside the formatted amount of an instruction type. Ex. \"Amount: 0.1 SOL\""
    )
    public static let solanaUnknownInstructionName = NSLocalizedString(
      "wallet.solanaUnknownInstructionName",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unknown Instruction Type",
      comment:
        "The title displayed above an unknown instruction type for Solana Instruction details."
    )
    public static let solanaInstructionProgramId = NSLocalizedString(
      "wallet.solanaInstructionProgramId",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Program Id",
      comment:
        "The label displayed beside the Program Id for an instruction type we don't support decoding. Ex. \"Program Id: 1111223344aabbccd\""
    )
    public static let solanaInstructionAccounts = NSLocalizedString(
      "wallet.solanaInstructionAccounts",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Accounts",
      comment:
        "The label displayed beside the Accounts for an instruction type we don't support decoding. Ex. \"Accounts: <solana_public_key>\""
    )
    public static let solanaInstructionData = NSLocalizedString(
      "wallet.solanaInstructionData",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Data",
      comment:
        "The label displayed beside the Data for an instruction type we don't support decoding. Ex. \"Data: [1, 20, 3, 5, 50]]\""
    )
    public static let solanaInstructionAddressLookupAcc = NSLocalizedString(
      "wallet.solanaInstructionAddressLookupAcc",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Address Lookup Table Account",
      comment:
        "The label displayed beside the address for the Address Lookup Table Account. Ex. \"Address Lookup Table Account: B1A2...\""
    )
    public static let solanaInstructionAddressLookupIndex = NSLocalizedString(
      "wallet.solanaInstructionAddressLookupIndex",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Address Lookup Table Index",
      comment:
        "The label displayed beside the address for the Address Lookup Table Account. Ex. \"Address Lookup Table Index: 1\""
    )
    public static let solanaSignTransactionWarning = NSLocalizedString(
      "wallet.solanaSignTransactionWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Note that Brave can’t verify what will happen if you sign. A signature could authorize nearly any operation in your account or on your behalf, including (but not limited to) giving total control of your account and crypto assets to the site making the request. Only sign if you’re sure you want to take this action, and trust the requesting site.",
      comment:
        "The warning message to let users understand the risk of using Brave Wallet to sign any transaction."
    )
    public static let solanaSignTransactionDetails = NSLocalizedString(
      "wallet.solanaSignTransactionDetails",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Details",
      comment:
        "The title on top of a separater, and the transaction details will be displayed below the separater."
    )
    public static let signMessageRequestUnknownUnicodeWarning = NSLocalizedString(
      "wallet.signMessageRequestUnknownUnicodeWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Non-ASCII characters detected!",
      comment:
        "A warning message to tell users that the sign request message contains non-ascii characters."
    )
    public static let signMessageConsecutiveNewlineWarning = NSLocalizedString(
      "wallet.signMessageConsecutiveNewlineWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Consecutive newline characters detected!",
      comment:
        "A warning message to tell users that the sign request message contains consecutive new-line characters."
    )
    public static let signMessageShowUnknownUnicode = NSLocalizedString(
      "wallet.signMessageShowUnknownUnicode",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Show encoded message",
      comment:
        "The title of the button that users can click to display the sign request message in ASCII encoding."
    )
    public static let signMessageShowOriginalMessage = NSLocalizedString(
      "wallet.signMessageShowOriginalMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View original message",
      comment:
        "The title of the button that users can click to display the sign request message as its original content."
    )
    public static let networkFilterTitle = NSLocalizedString(
      "wallet.networkFilterTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select Networks",
      comment: "The title displayed on the view to filter by a network / all networks."
    )
    public static let userAssetSymbolNetworkDesc = NSLocalizedString(
      "wallet.userAssetSymbolNetwork",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ on %@",
      comment:
        "The description displayed below the token name on each row for the user assets. The first '%@' will be the token symbol, and the second '%@' will be the token's network name."
    )
    public static let nftDetailTokenID = NSLocalizedString(
      "wallet.nftDetailTokenID",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token ID",
      comment:
        "The title an entry that will be displayed in NFT detail. This entry to display this NFT's token Id value."
    )
    public static let nftDetailERC721 = NSLocalizedString(
      "wallet.nftDetailERC721",
      tableName: "BraveWallet",
      bundle: .module,
      value: "ERC 721",
      comment:
        "This is one type of token standard. And most likey this does not need to be translated"
    )
    public static let nftDetailSPL = NSLocalizedString(
      "wallet.nftDetailSPL",
      tableName: "BraveWallet",
      bundle: .module,
      value: "SPL",
      comment:
        "This is one type of NFT standard. And most likey this does not need to be translated"
    )
    public static let nftDetailTokenStandard = NSLocalizedString(
      "wallet.nftDetailTokenStandard",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token Standard",
      comment:
        "The title an entry that will be displayed in NFT detail. This entry to display this NFT's token standard"
    )
    public static let nftDetailBlockchain = NSLocalizedString(
      "wallet.nftDetailBlockchain",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Blockchain",
      comment:
        "The title an entry that will be displayed in NFT detail. This entry to display this NFT's blockchain"
    )
    public static let nftDetailDescription = NSLocalizedString(
      "wallet.nftDetailDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Description",
      comment:
        "The title of a section in NFT detail screen that will display NFT's description if there is any."
    )
    public static let nftDetailSendNFTButtonTitle = NSLocalizedString(
      "wallet.nftDetailSendNFTButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Send",
      comment:
        "The title of the button inside NFT detail screen, which user can click to start sending this NFT."
    )
    public static let nftDetailImageNotAvailable = NSLocalizedString(
      "wallet.nftDetailImageNotAvailable",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Image is not available",
      comment: "A placeholder text, indicates this NFT image was not able to be loaded."
    )
    public static let nftDetailSVGImageDisclaimer = NSLocalizedString(
      "wallet.nftDetailSVGImageDisclaimer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "The image rendered above may not exactly match the NFT",
      comment:
        "A disclaimer that appears at the bottom of a NFT detail screen which shows NFT image and other information."
    )
    public static let nftDetailOverview = NSLocalizedString(
      "wallet.nftDetailOverview",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Overview",
      comment: "The section header that displays the overview information of this NFT."
    )
    public static let nftDetailProperties = NSLocalizedString(
      "wallet.nftDetailProperties",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Properties",
      comment:
        "The section header that displays the all the properties/attributes information of this NFT."
    )
    public static let nftDetailOwnedBy = NSLocalizedString(
      "wallet.nftDetailOwnedBy",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Owned By",
      comment:
        "The title of the row under `Overview` section in NFT details screen. When this NFT has an owner."
    )
    public static let signTransactionSignRisk = NSLocalizedString(
      "wallet.signTransactionSignRisk",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign at your own risk",
      comment: "A title of a warning message that warns users the risk of signing a transaction."
    )
    public static let swapConfirmationTitle = NSLocalizedString(
      "wallet.swapConfirmationTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Confirm Order",
      comment:
        "A title displayed in transaction confirmation navigation bar for a Swap transaction."
    )
    public static let swapConfirmationYouSpend = NSLocalizedString(
      "wallet.swapConfirmationYouSpend",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You spend",
      comment:
        "A title displayed in transaction confirmation above the token being swapped from in a Swap transaction."
    )
    public static let swapConfirmationYoullReceive = NSLocalizedString(
      "wallet.swapConfirmationYoullReceive",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You'll receive",
      comment:
        "A title displayed in transaction confirmation above the token being swapped for in a Swap transaction."
    )
    public static let swapConfirmationNetworkFee = NSLocalizedString(
      "wallet.swapConfirmationNetworkFee",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Network Fee",
      comment:
        "A title displayed in transaction confirmation above the network / gas fee for a Swap transaction."
    )
    public static let swapConfirmationNetworkDesc = NSLocalizedString(
      "wallet.swapConfirmationNetworkDesc",
      tableName: "BraveWallet",
      bundle: .module,
      value: "on %@",
      comment:
        "The description displayed below the name of the token being swapped from and to. Tthe '%@' will be the token's network name."
    )
    public static let web3DomainOptionAsk = NSLocalizedString(
      "wallet.web3DomainOptionAsk",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Ask",
      comment:
        "One of the options for Brave to handle Ethereum/Solana Name Service domain name and IPFS scheme url. 'Ask' means Brave will ask user first before enable or disable resolving ENS/SNS domain name and IPFS scheme url."
    )
    public static let web3DomainOptionEnabled = NSLocalizedString(
      "wallet.web3DomainOptionEnabled",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enabled",
      comment:
        "One of the options for Brave to handle Ethereum/Solana Name Service domain name and IPFS scheme url. 'Enabled' means Brave will enable resolving ENS/SNS domain name and IPFS scheme url."
    )
    public static let web3DomainOptionDisabled = NSLocalizedString(
      "wallet.web3DomainOptionDisabled",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Disabled",
      comment:
        "One of the options for Brave to handle Ethereum/Solana Name Service domain name and IPFS scheme url. 'Disabled' means Brave will disable resolving ENS/SNS domain name and IPFS scheme url."
    )
    public static let ensResolveMethodTitle = NSLocalizedString(
      "wallet.ensResolveMethodTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Resolve Ethereum Name Service (ENS) Domain Names",
      comment: "The title for the options to resolve Ethereum Name service domain names."
    )
    public static let ensOffchainResolveMethodTitle = NSLocalizedString(
      "wallet.ensOffchainResolveMethodTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Allow ENS Offchain Lookup",
      comment: "The title for the options to allow Ethereum Name service domain names offchain."
    )
    public static let ensOffchainResolveMethodDescription = NSLocalizedString(
      "wallet.ensOffchainResolveMethodDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "[Learn more](%@) about ENS offchain lookup privacy considerations.",
      comment:
        "The description for the options to allow Ethereum Name service domain names offchain. '%@' will be replaced with a url to explain more about ENS offchain lookup."
    )
    public static let snsResolveMethodTitle = NSLocalizedString(
      "wallet.web3DomainOptionsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Resolve Solana Name Service (SNS) Domain Names",
      comment: "The title for the options to resolve Solana Name service domain names."
    )
    public static let udResolveMethodTitle = NSLocalizedString(
      "wallet.udResolveMethodTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Resolve Unstoppable Domains Domain Names",
      comment: "The title for the options to resolve Unstoppable Domains domain names."
    )
    public static let udResolveMethodDescription = NSLocalizedString(
      "wallet.udResolveMethodDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "[Learn more](%@) about Unstoppable Domains privacy considerations.",
      comment:
        "The description for the options to allow Unstoppable Domain domain names. '%@' will be replaced with a url to explain more about Unstoppable Domains."
    )
    public static let web3DomainOptionsHeader = NSLocalizedString(
      "wallet.web3DomainOptionsHeader",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Web3 Domains",
      comment: "The header for the options to resolve Solana Name service domain names."
    )
    public static let signedTransactionTitle = NSLocalizedString(
      "wallet.signedTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Signed",
      comment:
        "A title of transaction signed status view indicating this transaction has been signed."
    )
    public static let submittedTransactionTitle = NSLocalizedString(
      "wallet.submittedTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Submitted",
      comment:
        "A title of transaction signed status view indicating this transaction has been submitted to the network."
    )
    public static let confirmedTransactionTitle = NSLocalizedString(
      "wallet.confirmedTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Completed",
      comment:
        "A title of transaction confirmed status view indicating this transaction has been included in a block."
    )
    public static let failedTransactionTitle = NSLocalizedString(
      "wallet.failedTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction Failed",
      comment:
        "A title of transaction failed status view indicating this transaction has not been included in a block."
    )
    public static let signedTransactionDescription = NSLocalizedString(
      "wallet.signedTransactionDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Transaction has been signed and will be sent to network by dapps and awaits confirmation.",
      comment:
        "A description explains signed transaction will be sent to network by dapps and awaits confirmation."
    )
    public static let submittedTransactionDescription = NSLocalizedString(
      "wallet.submittedTransactionDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction has been successfully sent to the network and awaits confirmation.",
      comment:
        "A description explains submitted transaction has been sent to the netwokr and awaits confirmation."
    )
    public static let confirmedTransactionDescription = NSLocalizedString(
      "wallet.confirmedTransactionDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Transaction has been successfully included in a block. To avoid the risk of double spending, we recommend waiting for block confirmations.",
      comment: "A description explains confirmed transaction has been included in a block."
    )
    public static let failedTransactionDescription = NSLocalizedString(
      "wallet.failedTransactionDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Transaction was failed due to a large price movement. Increase slippage tolerance to succeed at a larger price movement.",
      comment: "A description explains transaction is failed."
    )
    public static let confirmedTransactionReceiptButtonTitle = NSLocalizedString(
      "wallet.confirmedTransactionReceiptButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Receipt",
      comment: "A title of a button which can open a confirmed transaction details screen."
    )
    public static let confirmedTransactionCloseButtonTitle = NSLocalizedString(
      "wallet.confirmedTransactionCloseButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Close",
      comment: "A title of a button which will close this transaction status view."
    )
    public static let failedTransactionViewErrorButtonTitle = NSLocalizedString(
      "wallet.failedTransactionViewErrorButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "View Error",
      comment: "A title of a button which will a new view to display the error."
    )
    public static let failedTransactionErrorViewTitle = NSLocalizedString(
      "wallet.failedTransactionErrorViewTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Error Message",
      comment: "A title of the view that will display the error message."
    )
    public static let failedTransactionErrorViewDescription = NSLocalizedString(
      "wallet.failedTransactionErrorViewDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Please save the error message for future reference.",
      comment: "A description of the view that will display the error message."
    )
    public static let cancelERC20ApprovalTxRevertMsg = NSLocalizedString(
      "wallet.cancelERC20ApprovalTxRevertMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "allowance has not been approved under",
      comment: "A part of the message in a transaction status screen. It will follow the amount of tokens that users were attempt to approve but cancelled after. For exapmple: 10 ETH allowance has not been approve under account 0x345...436"
    )
    public static let cancelTxRevertMsg = NSLocalizedString(
      "wallet.cancelTxRevertMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "has been reverted to",
      comment: "A part of the message in a transaction status screen. It will follow the amount of tokens that users were attempt to send but cancelled after. For exapmple: 10 ETH has been reverted to account 0x345...436"
    )
    public static let cancellingTransactionTitle = NSLocalizedString(
      "wallet.cancellingTransactionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Cancelling transaction",
      comment: "A title of transaction status view indicating this transaction is submitted to chain which is cancelling another transaction."
    )
    public static let cancellingTransactionDescription = NSLocalizedString(
      "wallet.cancellingTransactionDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Cancellation request submitted",
      comment: "A description of transaction status view indicating this transaction is submitted to chain which is cancelling another transaction."
    )
    public static let confirmedTransactionCancellationTitle = NSLocalizedString(
      "wallet.confirmedTransactionCancellationTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Transaction cancelled!",
      comment: "A title of transaction status view indicating this transaction is confirmed on the chain which has cancelled another transaction."
    )
    public static let confirmedTransactionCancellationAccount = NSLocalizedString(
      "wallet.confirmedTransactionCancellationAccount",
      tableName: "BraveWallet",
      bundle: .module,
      value: "account",
      comment: "When a cancellation transaction has been confirmed, we will display which account the amount of value has been reverted to. For example: 10 ETH has been reverted to account 0x123...456"
    )
    public static let cancelTransactionStatusButtonTitle = NSLocalizedString(
      "wallet.cancelTransactionStatusButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Cancel transaction",
      comment: "The button title that for user to cancel a submitted transaction inside transaction status view."
    )
    public static let cancelTransactionStatusConfirmationDescription = NSLocalizedString(
      "wallet.cancelTransactionStatusConfirmationDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "A new transaction will be created to cancel your existing transaction",
      comment: "The description inside the view when asking user to confirm they want to cancel a transaction."
    )
    public static let ensOffchainGatewayTitle = NSLocalizedString(
      "wallet.ensOffchainGatewayTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave supports using offchain gateways to resolve .eth domains.",
      comment:
        "Title shown send address / ENS domain when requesting to do an ENS off chain lookup."
    )
    public static let ensOffchainGatewayDesc = NSLocalizedString(
      "wallet.ensOffchainGatewayDesc",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "It looks like you've entered an ENS address. We'll need to use a third-party resolver to resolve this request. This helps ensure your .eth domain isn't leaked, and that your transaction is secure.",
      comment:
        "Description shown send address / ENS domain when requesting to do an ENS off chain lookup."
    )
    public static let ensOffchainGatewayButton = NSLocalizedString(
      "wallet.ensOffchainGatewayButton",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Use ENS Domain",
      comment: "Button title when requesting to do an ENS off chain lookup."
    )
    public static let web3DomainInterstitialPageTAndU = NSLocalizedString(
      "wallet.web3DomainInterstitialPageTAndU",
      tableName: "BraveWallet",
      bundle: .module,
      value: "terms of use",
      comment:
        "Hyper link copy displayed as part of 'snsDomainInterstitialPageDescription'. It will redirect user to the 'terms of use' webpage of the server that Brave uses to resolve SNS domain."
    )
    public static let web3DomainInterstitialPagePrivacyPolicy = NSLocalizedString(
      "wallet.web3DomainInterstitialPagePrivacyPolicy",
      tableName: "BraveWallet",
      bundle: .module,
      value: "privacy policy",
      comment:
        "Hyper link copy displayed as part of 'snsDomainInterstitialPageDescription'. It will redirect user to the privay policy webpage of the server that Brave uses to resolve SNS domain."
    )
    public static let web3DomainInterstitialPageButtonDisable = NSLocalizedString(
      "wallet.web3DomainInterstitialPageButtonDisable",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Disable",
      comment:
        "Title on the button that users can click to disable Brave to resolve the ENS/SNS domain they entered."
    )
    // SNS
    public static let snsDomainInterstitialPageTitle = NSLocalizedString(
      "wallet.snsDomainInterstitialPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enable Support of Solana Name Service (SNS) in Brave?",
      comment:
        "Title displayed when users chose Brave to ask them if they want the SNS to be resolved every time they enter one."
    )
    public static let snsDomainInterstitialPageDescription = NSLocalizedString(
      "wallet.snsDomainInterstitialPageDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Brave will use a third-party to resolve .sol domain names. Brave hides your IP address. If you enable this, the third-party will see that someone is trying to visit these .sol domains, but nothing else. For more information about which third-parties we use and their privacy policies, please see our <a href=%@>help page</a>.",
      comment:
        "Description displayed when users chose Brave to ask them if they want the SNS to be resolved every time they enter one. The first '%@' will be replaced with a link to Brave's wiki page which will link to the providersterms of use page and privacy policy page. The last '%@' will be replaced with the value of 'snsDomainInterstitialPagePrivacyPolicy'."
    )
    public static let snsDomainInterstitialPageButtonProceed = NSLocalizedString(
      "wallet.snsDomainInterstitialPageButtonProceed",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Proceed using an SNS server",
      comment:
        "Title on the button that users can click to enable Brave to resolve the SNS domain they entered."
    )
    // ENS
    public static let ensDomainInterstitialPageTitle = NSLocalizedString(
      "wallet.ensDomainInterstitialPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enable Support of Ethereum Name Service (ENS) in Brave?",
      comment:
        "Title displayed when users chose Brave to ask them if they want the ENS domain to be resolved every time they enter one."
    )
    public static let ensDomainInterstitialPageDescription = NSLocalizedString(
      "wallet.ensDomainInterstitialPageDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Brave will be using Infura to resolve .eth domain names that are on Ethereum Name Service (ENS). Brave hides your IP address. If you enable this, Infura will see that someone is trying to visit these .eth domains but nothing else. See Infura's <a href=%@>%@</a> and <a href=%@>%@</a>.",
      comment:
        "Description displayed when users chose Brave to ask them if they want the ENS to be resolved every time they enter one. The first '%@' will be replaced with a link to Infura's terms of use page. The second '%@' will be replaced with the value of 'Web3DomainInterstitialPageTAndU'. The third '%@' will be replaced with a link to Infura's privacy policy page. The last '%@' will be replaced with the value of 'Web3DomainInterstitialPagePrivacyPolicy'."
    )
    public static let ensDomainInterstitialPageButtonProceed = NSLocalizedString(
      "wallet.ensDomainInterstitialPageButtonProceed",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Proceed using Infura server",
      comment:
        "Title on the button that users can click to enable Brave to resolve the ENS domain they entered."
    )
    // ENS offchain
    public static let ensOffchainDomainInterstitialPageTitle = NSLocalizedString(
      "wallet.ensOffchainDomainInterstitialPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enable support of Ethereum Name Service (ENS) offchain lookup in Brave?",
      comment:
        "Title displayed when users chose Brave to ask them if they want the ENS Offchain domain to be resolved every time they enter one."
    )
    public static let ensOffchainDomainInterstitialPageDescription = NSLocalizedString(
      "wallet.ensOffchainDomainInterstitialPageDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "This .eth domain name is stored offchain and will be resolved by a third party gateway. If you enable ENS offchain lookup, the third party gateway will see that you're trying to visit the .eth domain. <a href=%@>%@</a>.",
      comment:
        "Description displayed when users chose Brave to ask them if they want the ENS to be resolved every time they enter one. The first '%@' will be replaced with a link to Brave Wallet's offchain lookup privacy considerations. The second '%@' will be replaced with the value of 'ensOffchainDomainInterstitialLearnMore'."
    )
    public static let ensOffchainDomainInterstitialPageButtonProceed = NSLocalizedString(
      "wallet.ensOffchainDomainInterstitialPageButtonProceed",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Proceed with offchain lookup",
      comment:
        "Title on the button that users can click to enable Brave to resolve the ENS offchain lookup for the domain they entered."
    )
    // Unstoppable Domains
    public static let udDomainInterstitialPageTitle = NSLocalizedString(
      "wallet.udDomainInterstitialPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enable support of Unstoppable Domains in Brave?",
      comment:
        "Title displayed when users chose Brave to ask them if they want the Unstoppable Domains domain to be resolved every time they enter one."
    )
    public static let udDomainInterstitialPageDescription = NSLocalizedString(
      "wallet.udDomainInterstitialPageDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Brave will be using Infura to resolve .crypto (and also %@) domain names that are on Unstoppable Domains. Brave hides your IP address. If you enable this, Infura will see that someone is trying to visit these domains but nothing else. See Infura's <a href=%@>%@</a> and <a href=%@>%@</a>.",
      comment:
        "Description displayed when users chose Brave to ask them if they want the Unstoppable Domains to be resolved every time they enter one. The first '%@' will be replaced with a list of supported TLDs like '.x' or '.bitcoin'. The second '%@' be replaced with a link to Infura's terms of use page. The third '%@' will be replaced with the value of 'Web3DomainInterstitialPageTAndU'. The fourth '%@' will be replaced with a link to Infura's privacy policy page. The last '%@' will be replaced with the value of 'Web3DomainInterstitialPagePrivacyPolicy'."
    )
    public static let activityPageTitle = NSLocalizedString(
      "wallet.activityPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Activity",
      comment:
        "The title of the tab that will display user's transaction activity for all accounts."
    )
    public static let activityPageEmptyTitle = NSLocalizedString(
      "wallet.activityPageEmptyTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No transactions to view yet",
      comment: "The title when showing no transactions inside Activity tab."
    )
    public static let activityPageEmptyDescription = NSLocalizedString(
      "wallet.activityPageEmptyDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Once you make a transaction, you'll be able to view it here.",
      comment: "The description when showing no transactions inside Activity tab."
    )
    public static let nftPageTitle = NSLocalizedString(
      "wallet.nftPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "NFT",
      comment: "The title of the tab that will display user's visible NFT assets."
    )
    public static let nftPageEmptyTitle = NSLocalizedString(
      "wallet.nftPageEmptyTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No NFTs here yet.",
      comment: "The title of the empty state inside NFT tab."
    )
    public static let nftInvisiblePageEmptyTitle = NSLocalizedString(
      "wallet.nftInvisiblePageEmptyTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No hidden NFTs here yet.",
      comment: "The title of the empty state inside NFT tab under Hidden group."
    )
    public static let nftPageEmptyDescription = NSLocalizedString(
      "wallet.nftPageEmptyDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Ready to add some? Just tap the button below to import.",
      comment: "The description of the empty state inside NFT tab."
    )
    public static let marketPageTitle = NSLocalizedString(
      "wallet.marketPageTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Market",
      comment: "The page title that will display the top 250 tokens from the market via CoinGecko"
    )
    public static let coinMarketInformation = NSLocalizedString(
      "wallet.coinMarketInformation",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Information",
      comment: "The header title of the section that display coin market information."
    )
    public static let coinMarketRank = NSLocalizedString(
      "wallet.coinMarketRank",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Rank",
      comment: "The title of the rank of this coin from market."
    )
    public static let coinMarket24HVolume = NSLocalizedString(
      "wallet.coinMarket24HVolume",
      tableName: "BraveWallet",
      bundle: .module,
      value: "24h Volume",
      comment: "The title of the 24h volume of this coin from market."
    )
    public static let coinMarketMarketCap = NSLocalizedString(
      "wallet.coinMarketMarketCap",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Market Cap",
      comment: "The title of the market cap of this coin from market."
    )
    public static let coinMarketEmptyMsg = NSLocalizedString(
      "wallet.coinMarketEmptyMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No market information available yet",
      comment:
        "The message will be displayed when there is no coin market loaded or having an error."
    )
    public static let web3SettingsEnableNFTDiscovery = NSLocalizedString(
      "wallet.web3SettingsEnableNFTDiscovery",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Enable NFT Discovery",
      comment:
        "The title of the toggle for user to enable/disable NFT discovery inside Web3 settings."
    )
    public static let web3SettingsEnableNFTDiscoveryFooter = NSLocalizedString(
      "wallet.web3SettingsEnableNFTDiscoveryFooter",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Automatically add NFTs you own to the Wallet using third party APIs. [Learn more](%@)",
      comment:
        "The footer of the toggle for user to enable/disable NFT discovery inside Web3 settings."
    )
    public static let nftDiscoveryCalloutTitle = NSLocalizedString(
      "wallet.nftDiscoveryCalloutTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Want your NFTs displayed automatically?",
      comment:
        "The title of the alert that asks users either to enable NFT discovery or import manually."
    )
    public static let nftDiscoveryCalloutDescription = NSLocalizedString(
      "wallet.nftDiscoveryCalloutDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Brave Wallet can use a third-party service to automatically display your NFTs. Brave will share your wallet addresses with SimpleHash to provide this service. Learn more.",
      comment:
        "The title of the alert that asks users either to enable NFT discovery or import manually. `SimpleHash` is the third-party service name, so it does not need to be translated."
    )
    public static let nftDiscoveryCalloutDescriptionLearnMore = NSLocalizedString(
      "wallet.nftDiscoveryCalloutDescriptionLearnMore",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Learn more",
      comment:
        "This is the same `Learn more` at the end of `nftDiscoveryCalloutDescription`, but we need a separat translation to detect the range of it in order to build some attributed strings."
    )
    public static let nftDiscoveryCalloutDisable = NSLocalizedString(
      "wallet.nftDiscoveryCalloutDisable",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No thanks, I'll do it manually",
      comment: "The title of the button that user clicks to disable NFT discovery."
    )
    public static let nftDiscoveryCalloutEnable = NSLocalizedString(
      "wallet.nftDiscoveryCalloutEnable",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Yes, proceed",
      comment: "The title of the button that user clicks to enable NFT discovery."
    )
    public static let nftEmptyImportNFT = NSLocalizedString(
      "wallet.nftEmptyImportNFT",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Import NFT",
      comment: "The title of the button that user clicks to add his/her first NFT"
    )
    public static let nftCollected = NSLocalizedString(
      "wallet.nftCollected",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Collected",
      comment:
        "The title of one of the dropdown options to group NFTs. This group will display all user's visible NFTs."
    )
    public static let nftHidden = NSLocalizedString(
      "wallet.nftHidden",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hidden",
      comment:
        "The title of one of the dropdown options to group NFTs. This group will display all user's hidden NFTs."
    )
    public static let nftSpam = NSLocalizedString(
      "wallet.nftSpam",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Junk",
      comment: "The title of an overlay on top left of the junk NFT grid."
    )
    public static let nftUnhide = NSLocalizedString(
      "wallet.nftUnhide",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unhide",
      comment: "The title of context button for user to unhide visible NFT."
    )
    public static let nftUnspam = NSLocalizedString(
      "wallet.nftUnspam",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Mark as Not Junk",
      comment: "The title of context button for user to unspam a NFT."
    )
    public static let nftRemoveFromWallet = NSLocalizedString(
      "wallet.nftRemoveFromWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Don't Show in Wallet",
      comment: "The title of context button for user to do not show a NFT in wallet at all."
    )
    public static let nftRemoveFromWalletAlertTitle = NSLocalizedString(
      "wallet.nftRemoveFromWalletAlertTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Remove from Brave Wallet?",
      comment: "The title of the alert when user attempts to remove an NFT from wallet."
    )
    public static let nftRemoveFromWalletAlertDescription = NSLocalizedString(
      "wallet.nftRemoveFromWalletAlertDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "NFT will be removed from Brave Wallet but will remain on the blockchain. If you remove it, then change your mind, you'll need to import it again manually.",
      comment: "The description of the alert when user attempts to remove an NFT from wallet."
    )
    public static let selectTokenToSendTitle = NSLocalizedString(
      "wallet.selectTokenToSendTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select a Token to Send",
      comment:
        "The title of the view that lets the user select a token & account at the same time in the Send crypto view."
    )
    public static let selectTokenToSendNoTokens = NSLocalizedString(
      "wallet.selectTokenToSendNoTokens",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No available tokens",
      comment:
        "The description of the view that lets the user select a token & account at the same time in the Send crypto view when there are no available tokens."
    )
    public static let showZeroBalances = NSLocalizedString(
      "wallet.showZeroBalances",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Show Zero Balances",
      comment: "The title of a button that updates the filter to show tokens with zero balances."
    )
    public static let hideZeroBalances = NSLocalizedString(
      "wallet.hideZeroBalances",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hide Zero Balances",
      comment: "The title of a button that updates the filter to hide tokens with zero balances."
    )
    public static let selectAllButtonTitle = NSLocalizedString(
      "wallet.selectAllButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select All",
      comment: "The title of a button that Selects all visible options."
    )
    public static let deselectAllButtonTitle = NSLocalizedString(
      "wallet.deselectAllButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Deselect All",
      comment: "The title of a button that Deselects all visible options."
    )
    public static let filtersAndDisplaySettings = NSLocalizedString(
      "wallet.filtersAndDisplaySettings",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Filters and Display Settings",
      comment: "The title of the modal for filtering Portfolio and NFT views."
    )
    public static let lowToHighSortOption = NSLocalizedString(
      "wallet.lowToHighSortOption",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Low to High",
      comment:
        "The title of the sort option that sorts lowest value to highest value. Used in Portfolio/NFT filters and display settings."
    )
    public static let highToLowSortOption = NSLocalizedString(
      "wallet.highToLowSortOption",
      tableName: "BraveWallet",
      bundle: .module,
      value: "High to Low",
      comment:
        "The title of the sort option that sorts highest value to lowest value. Used in Portfolio/NFT filters and display settings."
    )
    public static let aToZSortOption = NSLocalizedString(
      "wallet.aToZSortOption",
      tableName: "BraveWallet",
      bundle: .module,
      value: "A to Z",
      comment:
        "The title of the sort option that sorts alphabetically from A to Z. Used in Portfolio/NFT filters and display settings."
    )
    public static let zToASortOption = NSLocalizedString(
      "wallet.zToASortOption",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Z to A",
      comment:
        "The title of the sort option that sorts alphabetically from Z to A. Used in Portfolio/NFT filters and display settings."
    )
    public static let sortAssetsTitle = NSLocalizedString(
      "wallet.sortAssetsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sort Assets",
      comment:
        "The label of the sort option that sorts assets by their fiat value. Used in Portfolio/NFT filters and display settings."
    )
    public static let sortAssetsDescription = NSLocalizedString(
      "wallet.sortAssetsDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sort by fiat value or name",
      comment:
        "The description label of the sort option that sorts assets by their fiat value, shown below the title. Used in Portfolio/NFT filters and display settings."
    )
    public static let hideSmallBalancesTitle = NSLocalizedString(
      "wallet.hideSmallBalancesTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hide Small Balances",
      comment:
        "The label of the filter option that hides assets if their fiat value is below $0.05. Used in Portfolio/NFT filters and display settings."
    )
    public static let hideSmallBalancesDescription = NSLocalizedString(
      "wallet.hideSmallBalancesDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Assets with value less than $0.05",
      comment:
        "The description label of the filter option that hides assets if their fiat value is below $0.05, shown below the title. Used in Portfolio/NFT filters and display settings."
    )
    public static let hideUnownedNFTsTitle = NSLocalizedString(
      "wallet.hideUnownedNFTsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hide Unowned",
      comment:
        "The label of the filter option that hides NFTs if they are not owned by the user. Used in Portfolio/NFT filters and display settings."
    )
    public static let hideUnownedNFTsDescription = NSLocalizedString(
      "wallet.hideUnownedNFTsDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Hide NFTs that have no balance",
      comment:
        "The description label of the filter option that hides NFTs if they are not owned by the user. Used in Portfolio/NFT filters and display settings."
    )
    public static let showNFTNetworkLogoTitle = NSLocalizedString(
      "wallet.showNFTNetworkLogoTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Network Logo",
      comment:
        "The label of the filter option that hides the network logo on NFTs displayed in the grid. Used in NFT filters and display settings."
    )
    public static let showNFTNetworkLogoDescription = NSLocalizedString(
      "wallet.showNFTNetworkLogoDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Show network logo on NFTs",
      comment:
        "The description label of the filter option that hides the network logo on NFTs displayed in the grid. Used in NFT filters and display settings."
    )
    public static let selectAccountsTitle = NSLocalizedString(
      "wallet.selectAccountsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select Accounts",
      comment:
        "The label of the filter option that allows users to select which accounts to filter assets by. Used in Portfolio/NFT filters and display settings."
    )
    public static let selectAccountsDescription = NSLocalizedString(
      "wallet.selectAccountsDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select accounts to filter by",
      comment:
        "The description label of the filter option that allows users to select which accounts to filter assets by, shown below the title. Used in Portfolio/NFT filters and display settings."
    )
    public static let allAccountsLabel = NSLocalizedString(
      "wallet.allAccountsLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "All accounts",
      comment:
        "The label of badge beside the filter option that allows users to select which accounts to filter assets by, when all accounts are selected. Used in Portfolio/NFT filters and display settings."
    )
    public static let selectNetworksTitle = NSLocalizedString(
      "wallet.selectNetworksTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select Networks",
      comment:
        "The label of the filter option that allows users to select which networks to filter assets by. Used in Portfolio/NFT filters and display settings."
    )
    public static let selectNetworksDescription = NSLocalizedString(
      "wallet.selectNetworksDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Select networks to filter by",
      comment:
        "The description label of the filter option that allows users to select which networks to filter assets by, shown below the title. Used in Portfolio/NFT filters and display settings."
    )
    public static let allNetworksLabel = NSLocalizedString(
      "wallet.allNetworksLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "All networks",
      comment:
        "The label of badge beside the filter option that allows users to select which networks to filter assets by, when all networks are selected. Used in Portfolio/NFT filters and display settings."
    )
    public static let saveChangesButtonTitle = NSLocalizedString(
      "wallet.saveChangesButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Save Changes",
      comment:
        "The title of the label of the button to save all changes. Used in Portfolio/NFT filters and display settings."
    )
    public static let groupByTitle = NSLocalizedString(
      "wallet.groupByTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Group By",
      comment:
        "The label of the sort option that groups assets by the selected filter value. Used in Portfolio/NFT filters and display settings."
    )
    public static let groupByDescription = NSLocalizedString(
      "wallet.groupByDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Group assets by",
      comment:
        "The description label of the sort option that groups assets by the selected filter value, shown below the title. Used in Portfolio/NFT filters and display settings."
    )
    public static let groupByNoneOptionTitle = NSLocalizedString(
      "wallet.groupByNoneOptionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "None",
      comment:
        "The title of the sort option that does not group assets. Used in Portfolio/NFT filters and display settings."
    )
    public static let groupByAccountsOptionTitle = NSLocalizedString(
      "wallet.groupByAccountsOptionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Accounts",
      comment:
        "The title of the sort option that groups assets by each account. Used in Portfolio/NFT filters and display settings."
    )
    public static let groupByNetworksOptionTitle = NSLocalizedString(
      "wallet.groupByNetworksOptionTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Networks",
      comment:
        "The title of the sort option that groups assets by each network. Used in Portfolio/NFT filters and display settings."
    )
    public static let portfolioEmptyStateTitle = NSLocalizedString(
      "wallet.portfolioEmptyStateTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "No available assets",
      comment: "The title of the empty state in Portfolio when no assets are shown."
    )
    public static let portfolioEmptyStateDescription = NSLocalizedString(
      "wallet.portfolioEmptyStateDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value:
        "Deposit or purchase tokens to get started. If you don't see tokens from an imported account, check the filters and display settings. Unknown tokens may need to be added as custom assets.",
      comment: "The title of the empty state in Portfolio when no assets are shown."
    )
    public static let internalErrorMessage = NSLocalizedString(
      "wallet.internalErrorMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "An internal error has occurred",
      comment: "The title of a button that Deselects all visible options."
    )
    public static let signInWithBraveWallet = NSLocalizedString(
      "wallet.signInWithBraveWallet",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign in with Brave Wallet",
      comment: "The title of the view shown above a Sign In With Ethereum request."
    )
    public static let securityRiskDetectedTitle = NSLocalizedString(
      "wallet.securityRiskDetectedTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Security Risk Detected",
      comment:
        "The title of the view shown when a security issue is detected with a Sign In With Ethereum request."
    )
    public static let signInWithBraveWalletMessage = NSLocalizedString(
      "wallet.signInWithBraveWalletMessage",
      tableName: "BraveWallet",
      bundle: .module,
      value: "You are signing into %@. Brave Wallet will share your wallet address with %@.",
      comment:
        "The title of the view shown when a security issue is detected with a Sign In With Ethereum request."
    )
    public static let seeDetailsButtonTitle = NSLocalizedString(
      "wallet.seeDetailsButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "See details",
      comment: "The title of the button to show details."
    )
    public static let siweMessageLabel = NSLocalizedString(
      "wallet.siweMessageLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Message",
      comment: "The label displayed above the Sign In With Ethereum message."
    )
    public static let siweResourcesLabel = NSLocalizedString(
      "wallet.siweResourcesLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Resources",
      comment: "The label displayed above the Sign In With Ethereum resources."
    )
    public static let siweSignInButtonTitle = NSLocalizedString(
      "wallet.siweSignInButtonTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign In",
      comment:
        "The label displayed on the button to sign in for Sign In With Ethereum/Brave Wallet requests."
    )
    public static let siweOriginLabel = NSLocalizedString(
      "wallet.siweOriginLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Origin",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the origin."
    )
    public static let siweAddressLabel = NSLocalizedString(
      "wallet.siweAddressLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Address",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the address."
    )
    public static let siweURILabel = NSLocalizedString(
      "wallet.siweURILabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "URI",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the URI."
    )
    public static let siweVersionLabel = NSLocalizedString(
      "wallet.siweVersionLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Version",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the version."
    )
    public static let siweChainIDLabel = NSLocalizedString(
      "wallet.siweChainIDLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Chain ID",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the chain ID."
    )
    public static let siweIssuedAtLabel = NSLocalizedString(
      "wallet.siweIssuedAtLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Issued At",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the issued at date."
    )
    public static let siweExpirationTimeLabel = NSLocalizedString(
      "wallet.siweExpirationTimeLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Expiration Time",
      comment:
        "The label displayed in details for Sign In With Ethereum/Brave Wallet requests beside the expiration time."
    )
    public static let siweNonceLabel = NSLocalizedString(
      "wallet.siweNonceLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Nonce",
      comment:
        "The label displayed in details view for Sign In With Ethereum/Brave Wallet requests beside the nonce."
    )
    public static let siweStatementLabel = NSLocalizedString(
      "wallet.siweStatementLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Statement",
      comment:
        "The label displayed in details view for Sign In With Ethereum/Brave Wallet requests beside the statement."
    )
    public static let siweDetailsTitle = NSLocalizedString(
      "wallet.siweDetailsTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Details",
      comment: "The title of the details view for Sign In With Ethereum/Brave Wallet requests."
    )
    public static let transactionSummaryIntentLabel = NSLocalizedString(
      "wallet.transactionSummaryIntentLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%@ from",
      comment:
        "The label used to describe a transaction type. Used like 'Send from' or 'Approved from'."
    )
    public static let transactionSummarySwapOn = NSLocalizedString(
      "wallet.transactionSummarySwapOn",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap on",
      comment: "The label to describe an Swap transaction."
    )
    public static let transactionSummarySolanaSwap = NSLocalizedString(
      "wallet.transactionSummarySolanaSwap",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Solana Swap",
      comment: "The label to describe an Solana Swap transaction."
    )
    public static let search = NSLocalizedString(
      "wallet.search",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Search",
      comment: "The label as a placeholder in search fields."
    )
    public static let securityTitle = NSLocalizedString(
      "wallet.securityTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Security",
      comment: "The title used in the row opening DApp settings in Account Details."
    )
    public static let accountSecurityDescription = NSLocalizedString(
      "wallet.accountSecurityDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Connected Sites and Allowances",
      comment:
        "The description used below Security title in the row opening DApp settings in Account Details."
    )
    public static let assetsSingularDescription = NSLocalizedString(
      "wallet.assetsSingularDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%d asset",
      comment: "The description of the assets row in Account Details when user has 1 Asset."
    )
    public static let assetsDescription = NSLocalizedString(
      "wallet.assetsDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%d assets",
      comment:
        "The description of the assets row in Account Details when user has zero or multiple Assets."
    )
    public static let nftsSingularDescription = NSLocalizedString(
      "wallet.nftsSingularDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%d NFT",
      comment: "The description of the NFTs row in Account Details when user has 1 NFT."
    )
    public static let nftsDescription = NSLocalizedString(
      "wallet.nftsDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%d NFTs",
      comment:
        "The description of the NFTs row in Account Details when user has zero or multiple NFTs."
    )
    public static let transactionsSingularDescription = NSLocalizedString(
      "wallet.transactionsSingularDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%d transactions",
      comment:
        "The description of the transactions row in Account Details when user has 1 Transaction."
    )
    public static let transactionsDescription = NSLocalizedString(
      "wallet.transactionsDescription",
      tableName: "BraveWallet",
      bundle: .module,
      value: "%d transactions",
      comment:
        "The description of the transactions row in Account Details when user has zero or multiple Transactions."
    )
    public static let depositAddressCopy = NSLocalizedString(
      "wallet.depositAddressCopy",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Copy",
      comment:
        "The button title for user to click to copy the account address for deposit."
    )
    public static let depositEthDisclosure = NSLocalizedString(
      "wallet.depositEthDisclosure",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This is Ethereum wallet which support Ethereum Network and level 2 network such as %@",
      comment:
        "The disclosure at the bottom of the deposit view for ETH address indicates ETH wallet supports Ethereum Mainnet as well as the level 2 EVM. `%@` will be replaced with current supported level 2 EVM network names."
    )
    public static let btcPendingBalancesBannerDesc = NSLocalizedString(
      "wallet.btcPendingBalancesBannerDesc",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Some balances may be unavailable",
      comment: "Banner description displayed below BTC rows displaying balance when there is a pending BTC balance."
    )
    public static let btcAvailableBalanceTitle = NSLocalizedString(
      "wallet.btcAvailableBalanceTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Available",
      comment: "Title displayed in row beside users available BTC balance."
    )
    public static let btcAvailableBalanceDesc = NSLocalizedString(
      "wallet.btcAvailableBalanceDesc",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Funds available for you to use.",
      comment: "Description displayed in row beside users available balance."
    )
    public static let btcPendingBalanceTitle = NSLocalizedString(
      "wallet.btcPendingBalanceTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Pending",
      comment: "Title displayed in row beside users pending BTC balance."
    )
    public static let btcPendingBalanceDesc = NSLocalizedString(
      "wallet.btcPendingBalanceDesc",
      tableName: "BraveWallet",
      bundle: .module,
      value: "A pending change in your wallet balance.",
      comment: "Description displayed in row beside users pending balance."
    )
    public static let btcTotalBalanceTitle = NSLocalizedString(
      "wallet.btcTotalBalanceTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Total",
      comment: "Title displayed in row beside users total BTC balance."
    )
    public static let btcTotalBalanceDesc = NSLocalizedString(
      "wallet.btcTotalBalanceDesc",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Your available funds plus any not-yet-confirmed transactions.",
      comment: "Description displayed in row beside users total balance."
    )
    public static let setDefaultNetwork = NSLocalizedString(
      "wallet.setDefaultNetwork",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Set as Default",
      comment:
        "One of the context menu option for user to tap to set a default network in wallet settings."
    )
    public static let btcOrdinalsUnsupportedWarning = NSLocalizedString(
      "wallet.btcOrdinalsUnsupportedWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Brave Wallet does not currently support Bitcoin NFTs (ordinals). Sending BTC from an address that has ordinals may result in its ordinals being transferred inadvertently.",
      comment: "A warning displayed in row before the Send button in Send Crypto view when a Bitcoin asset is selected."
    )
    public static let inputLabel = NSLocalizedString(
      "wallet.inputLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Input",
      comment:
        "The label displayed in details for Bitcoin Transaction Confirmation details panel beside/above the Input index."
    )
    public static let outputLabel = NSLocalizedString(
      "wallet.outputLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Output",
      comment:
        "The label displayed in details for Bitcoin Transaction Confirmation details panel beside/above the Output index."
    )
    public static let valueLabel = NSLocalizedString(
      "wallet.valueLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Value",
      comment:
        "The label displayed in details for Bitcoin Transaction Confirmation details panel beside/above the value."
    )
    public static let addressLabel = NSLocalizedString(
      "wallet.addressLabel",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Address",
      comment:
        "The label displayed in details for Bitcoin Transaction Confirmation details panel beside/above the address."
    )
    public static let bitcoinImportExtendedKeyWarning = NSLocalizedString(
      "wallet.bitcoinImportExtendedKeyWarning",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Only %@ extended keys are supported.",
      comment:
        "The label displayed in Add Account above the private key box when adding or importing a Bitcoin account. The '%@' is the standard supported for the selected network (for example \"Only zprv extended keys are supported.\")"
    )
    public static let editExistingNetworkAlertMsg = NSLocalizedString(
      "wallet.editExistingNetworkAlertMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Do you want to update the existing network?",
      comment:
        "A pop up message that will prompt to user before user confirms to edit the existing network."
    )
    public static let txFunctionTypeERC20Approve = NSLocalizedString(
      "wallet.txFunctionTypeERC20Approve",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approve",
      comment:
        "Blockchain function name for ERC-20 token approvals."
    )
    public static let txFunctionTypeTokenTransfer = NSLocalizedString(
      "wallet.txFunctionTypeTokenTransfer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Token Transfer",
      comment:
        "Blockchain function name for non-native token transfers."
    )
    public static let txFunctionTypeNFTTransfer = NSLocalizedString(
      "wallet.txFunctionTypeNFTTransfer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "NFT Transfer",
      comment:
        "Blockchain function name for NFT transfers"
    )
    public static let txFunctionTypeSafeTransfer = NSLocalizedString(
      "wallet.txFunctionTypeSafeTransfer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Safe Transfer",
      comment:
        "Blockchain function name for SafeTransferFrom in ERC-721 and ERC-1155 tokens."
    )
    public static let txFunctionTypeForwardFil = NSLocalizedString(
      "wallet.txFunctionTypeForwardFil",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Forward FIL",
      comment:
        "Blockchain function name for moving FIL from an  Ethereum-based f4 address to a Filecoin address of a different type. Don't need to translate the symbol `FIL`."
    )
    public static let txFunctionTypeSignAndSendDapp = NSLocalizedString(
      "wallet.txFunctionTypeSignAndSendDapp",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign and Send DApp transaction",
      comment:
        "Blockchain function name for signing and sending a Solana DApp transaction."
    )
    public static let txFunctionTypeSend = NSLocalizedString(
      "wallet.txFunctionTypeSend",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Send %@",
      comment:
        "Blockchain function name for send native token. `%@` will be replaced with native token symbol."
    )
    public static let txFunctionTypeSwap = NSLocalizedString(
      "wallet.txFunctionTypeSwap",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swap",
      comment:
        "Blockchain function name for swap."
    )
    public static let txFunctionTypeSplWithAssociatedTokenAccountCreation = NSLocalizedString(
      "wallet.txFunctionTypeSplWithAssociatedTokenAccountCreation",
      tableName: "BraveWallet",
      bundle: .module,
      value: "SPL Token Transfer With Associated Token Account Creation",
      comment:
        "Blockchain function name for Solana SPL token transfer with associated token account creation."
    )
    public static let txFunctionTypeCompressedNFTTransfer = NSLocalizedString(
      "wallet.txFunctionTypeCompressedNFTTransfer",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Compressed NFT Transfer",
      comment:
        "Blockchain function name for Solana compressed NFT transfers."
    )
    public static let txFunctionTypeOther = NSLocalizedString(
      "wallet.txFunctionTypeOther",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Other",
      comment:
        "Blockchain function name for an unknown transaction type or an FIL transaction."
    )
    public static let txFunctionTypeSignDappTransaction = NSLocalizedString(
      "wallet.txFunctionTypeSignDappTransaction",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sign DApp Transaction",
      comment:
        "Blockchain function name for Solana Sign DApp transactions."
    )
    public static let duplicationTokenError = NSLocalizedString(
      "wallet.duplicationTokenError",
      tableName: "BraveWallet",
      bundle: .module,
      value: "This token has already been added to your portfolio.",
      comment:
        "An error message that will get displayed when user is trying to add an existed custom token."
    )
    public static let txStatusSending = NSLocalizedString(
      "wallet.txStatusSending",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Sending",
      comment:
        "The transaction type (verb) in the submitted transaction status screen. This copy indicates this is a send transaction waitings for getting completed."
    )
    public static let txStatusSwapping = NSLocalizedString(
      "wallet.txStatusSwapping",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Swapping",
      comment:
        "The transaction type (verb) in the submitted transaction status screen. This copy indicates this is a swap transaction waitings for getting completed."
    )
    public static let txStatusApproving = NSLocalizedString(
      "wallet.txStatusApproving",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Approving",
      comment:
        "The transaction type (verb) in the submitted transaction status screen. This copy indicates this is a ERC20 Approval transaction waitings for getting completed."
    )
    public static let txStatusTo = NSLocalizedString(
      "wallet.txStatusTo",
      tableName: "BraveWallet",
      bundle: .module,
      value: "to",
      comment:
        "The word of preposition. Indicates the following account is the account receiving a certain amount of tokens. For example: Sending 0.1 ETH `to` Account 1."
    )
    public static let txStatusOn = NSLocalizedString(
      "wallet.txStatusOn",
      tableName: "BraveWallet",
      bundle: .module,
      value: "on",
      comment:
        "The word of preposition. Indicates some ERC20 token is approved on the following account. For example: Approving 0.1 ETH `on` Account 1."
    )
    public static let txStatusCompleted = NSLocalizedString(
      "wallet.txStatusCompleted",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Completed",
      comment:
        "The status inside transaction status screen, indicates this transaction is completed/confirmed on the blockchain."
    )
    public static let txStatusSentMsg = NSLocalizedString(
      "wallet.txStatusSentMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "has been sent to account",
      comment:
        "A message explains a send transaction is confirmed on the blockchain."
    )
    public static let txStatusERC20ApprovalMsg = NSLocalizedString(
      "wallet.txStatusERC20ApprovalMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "has been approved on your",
      comment:
        "A message explains an ERC20 Approval transaction is confirmed on the blockchain."
    )
    public static let txStatusSwappedMsg = NSLocalizedString(
      "wallet.txStatusSwappedMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "has been added to your",
      comment:
        "A message explains an ETH Swap transaction is confirmed on the blockchain."
    )
    public static let txStatusSendTxErrorTitle = NSLocalizedString(
      "wallet.txStatusSendTxErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unable to send",
      comment:
        "The title of the message when a send transaction failed to be confirmed on the blockchain."
    )
    public static let txStatusSendTxErrorMsg = NSLocalizedString(
      "wallet.txStatusSendTxErrorMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "There was an error attempting to send",
      comment:
        "The message after the title when a send transaction failed to be confirmed on the blockchain."
    )
    public static let txStatusSwapTxErrorTitle = NSLocalizedString(
      "wallet.txStatusSwapTxErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unable to swap",
      comment:
        "The title of the message when a swap transaction failed to be confirmed on the blockchain."
    )
    public static let txStatusSwapTxErrorMsg = NSLocalizedString(
      "wallet.txStatusSendTxErrorMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "There was an error attempting to swap",
      comment:
        "The message after the title when a swap transaction failed to be confirmed on the blockchain."
    )
    public static let txStatusERC20ApprovalTxErrorTitle = NSLocalizedString(
      "wallet.txStatusERC20ApprovalTxErrorTitle",
      tableName: "BraveWallet",
      bundle: .module,
      value: "Unable to approve",
      comment:
        "The title of the message when a ERC20 Approval transaction failed to be confirmed on the blockchain."
    )
    public static let txStatusERC20ApprovalTxErrorMsg = NSLocalizedString(
      "wallet.txStatusERC20ApprovalTxErrorMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value: "There was an error attempting to approve",
      comment:
        "The message after the title when an ERC20 Approval transaction failed to be confirmed on the blockchain."
    )
    public static let txStatusSignedConfirmedMsg = NSLocalizedString(
      "wallet.txStatusSignedConfirmedMsg",
      tableName: "BraveWallet",
      bundle: .module,
      value:  "Transaction has been signed and will be sent to network by dapps and await for confirmation.",
      comment:
        "The message when a sign transaction is completed/confirmed on the blockchain."
    )
    public static let txStatusViewInActivity = NSLocalizedString(
      "wallet.txStatusViewInActivity",
      tableName: "BraveWallet",
      bundle: .module,
      value:  "View in Activity",
      comment:
        "The title for the button. User would tap on this button to view the transaction status in the `Activity` tab."
    )
    public static let txStatusSubmittedDisclosure = NSLocalizedString(
      "wallet.txStatusSubmittedDisclosure",
      tableName: "BraveWallet",
      bundle: .module,
      value:  "You can safely dismiss this screen.",
      comment:
        "The text in the disclosure view in transaction status screen."
    )
  }
}
