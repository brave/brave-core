# How to push strings to Crowdin
Once your english strings are all up to date. These changes can be pushed to Crowdin for translation by running the following from Terminal:

```
cd l10n/
TOKEN=<token> ./push-strings-to-crowdin.sh
```

If there any issues pushing strings to Transifex then these issues are logged to ```output.log```

### The API token can be created from Crowdin Dev Portal 

# How to contribute translations via Transifex

There are several things you can do to help us internationalize Brave and provide a great experience for everybody

## Providing a translation

We manage all of our translations using Transifex. Here's how you can get started:
- [Create an account](https://www.transifex.com/signup/?join_project=brave-ios) with Transifex (it's free!)
- During the setup, it'll ask if you want to start your own project or join an existing project. Choose to join an existing project.
- Transifex will ask which languages you speak; filling this in is appreciated so that we have an accurate snapshot of the languages our contributors are familiar with.
- At this point, your account will be created and you can confirm your email.

You are now ready to join and help with translations or you can request a new language.
- Visit https://www.transifex.com/brave/brave-ios/
- In the top right, you can click "Join team".
- You can specify the languages you speak OR request a language which is not currently provided
- One of our contributors will be able to approve your access.

## How does translated text get back into the GitHub repository?
We generally pull in all languages files at the time we cut a release. That allows us to keep everything up to date in a scalable way.
For reference, here are a few pull requests where we've pulled in new language files

- https://github.com/brave/brave-ios/pull/???

## Making sure our code has all strings localized

### Prerequisites

* Open the Terminal application
* Type ```sudo easy_install pip```
* Type ```sudo -H pip install tornado```
* Type ```sudo -H pip install lxml```

Besides providing the actual translations themselves, it's important that the code tokenizes all strings shown to the user.

### Fixing existing known issues

You can search our existing issues and find places to contribute here:
https://github.com/brave/brave-ios/labels/l10n

### Properly modifying a string

Different files are used by different parts of the code. If you're not sure which file to edit, you should do a search or grep using another string in the same code you're looking at.

### Properly adding a new string

New strings should be statically added to the Strings.swift file in the `en_US` locale like so:

```
extension Strings {
    public static let someString = NSLocalizedString("SomeString", value: "Some String", comment: "Comments are used to bring context to the string to aide others")
}
```

These string changes can then be pushed to Transifex for translation by running the following from Terminal:

```
cd l10n/
USERNAME=<username> PASSWORD=<password> ./push-strings-to-transifex.sh
```

If there any issues pushing strings to Transifex then these issues are logged to ```output.log```

### How Transifex handles updates

Please check [what happens when you update files](https://docs.transifex.com/projects/updating-content#what-happens-when-you-update-files)
 in order to have an overall view of how Transifex handles updates of a source file.

### Referencing that new string

Inside your Swift code, you can get the localized values like so:

```
let someString = Strings.someString
```

### Importing the latest translations

You can import the latest translations from Transifex into the project by running the following from Terminal:

```
cd l10n/
USERNAME=<username> PASSWORD=<password> ./pull-translations-into-project.sh
```

If there any issues pulling translations into the Xcode project then these issues are logged to ```output.log```

**IMPORTANT:** Before importing the latest translations, please take a moment to look at [How does translated text get back into the GitHub repository?](#how-does-translated-text-get-back-into-the-github-repository?)

### Known limitations

* When importing translations the ```xcodebuild``` command-line tool:
    * does __not__ allow us to specify where localizations are imported into the project folder structure
    * adds duplicate .strings entries to the ```Copy Bundle Resources``` section of ```Build Phases```, which need to be manually removed

