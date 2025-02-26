### Properly modifying a string

Different files are used by different parts of the code. If you're not sure which file to edit, you should do a search or grep using another string in the same code you're looking at.

### Properly adding a new string

New strings should be statically added to the Strings.swift file in the `en_US` locale like so:

```
extension Strings {
    public static let someString = NSLocalizedString("SomeString", value: "Some String", comment: "Comments are used to bring context to the string to aide others")
}
```

### Referencing that new string

Inside your Swift code, you can get the localized values like so:

```
let someString = Strings.someString
```

### How to push strings to Crowdin

### Prerequisites
This script is using bash jq command-line tool for parsing JSON data.
Make sure jq is installed and added to your PATH
 
Once your english strings are all up to date. These changes can be pushed to Crowdin for translation by running the following from Terminal:

```
cd l10n/
TOKEN=<token> VERSION=<release_version> ./push-strings-to-crowdin.sh
```

If there any issues pushing strings to Crowdin then these issues are logged to ```output.log```
The `VERSION` is in the format of `version_number`. For example: `1_75_0`.

### Importing the latest translations from Crowdin
 
You can check if all translation is finished on Crowdin Dev Portal. Once it is finished, the translated xliff files can be downloaded and imported into the project by running in Terminal:

```
cd l10n/
TOKEN=<token> ./pull-translations-from-crowdin.sh
```

If there any issues downloading from Crowdin or importing strings to the project then these issues are logged to ```output.log```

### Note

The API token can be created from Crowdin Dev Portal 

### How does translated text get back into the GitHub repository?

We generally pull in all languages files at the time we cut a release. That allows us to keep everything up to date in a scalable way.
For reference, here are a few pull requests where we've pulled in new language files

- https://github.com/brave/brave-ios/pull/???

**IMPORTANT:** Before importing the latest translations, please take a moment to look at [How does translated text get back into the GitHub repository?](#how-does-translated-text-get-back-into-the-github-repository?)

### Known limitations

* When importing translations the ```xcodebuild``` command-line tool:
    * does __not__ allow us to specify where localizations are imported into the project folder structure
    * adds duplicate .strings entries to the ```Copy Bundle Resources``` section of ```Build Phases```, which need to be manually removed

