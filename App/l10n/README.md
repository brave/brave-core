## Localizing Strings

In this directory you will find a `Strings.swift`, this source is part of the `Strings` target in `Package.swift`.  To define a new string you can use one of the previous strings file such as `BraveStrings.swift` or `WalletStrings.swift` and use `NSLocalizedString` with the appropriate `tableName`.  The bundle you use **must be** `Bundle.strings` to ensure it knows where to load the `{tableName}.strings` files.

If you create a new Swift file that contains `NSLocalizedString` for a new module, please read the section below about adding it to the `l10n` project. The table name you use in your new Swift file should be unique and not be left blank (which will default to `Localizable`)

## Xcode Localization Export

Xcode currently does not provide a way to export localizations for products defined in a Swift Package, therefore we must introduce an unused framework within the main App project.

This new framework target is called `l10n` and is part of `App/Client.xcodeproj`. Source files that contain `NSLocalizedString` usage **must be added to this framework** (as a reference only) to ensure Xcode can find them and generate the appropriate `xliff` files needed to import in Transifex. (For more info on that, see `tools/README.md`).

The generated `.strings` files are also imported into this directory and will be automatically picked up by the `Strings` SPM target.

**Note**: The l10n framework does not need to compile or be added as a dependency since we only depend on `NSLocalizedString` usage.
