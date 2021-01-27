![Static Logo](docs/static.png)

[![Version](https://img.shields.io/github/release/venmo/Static.svg)](https://github.com/venmo/Static/releases) ![Status](https://travis-ci.org/venmo/Static.svg?branch=master) ![Swift Version](https://img.shields.io/badge/swift-4.2-orange.svg) [![Carthage compatible](https://img.shields.io/badge/Carthage-compatible-4BC51D.svg?style=flat)](https://github.com/Carthage/Carthage)

Simple static table views for iOS in Swift. Static's goal is to separate model data from presentation. `Row`s and `Section`s are your “view models” for your cells. You simply specify a cell class to use and that handles all of the presentation. See the [usage](#usage) section below for details.


## Version Compatibility

| Swift Version | Static Version |
| ------------- | -------------- |
| 5.0+          | 4.0.0          |
| 4.2+          | 3.0.1          |
| 3.2+          | 2.1            |
| 3.0.1         | 2.0.1          |
| 3.0           | 2.0            |
| 2.3           | 1.2            |
| 2.2           | 1.1            |
| 2.0 - 2.1     | 1.0            |


## Installation

### Carthage

[Carthage](https://github.com/carthage/carthage) is the recommended way to install Static. Add the following to your Cartfile:

``` ruby
github "venmo/Static"
```


### CocoaPods

[CocoaPods](http://cocoapods.org) is a dependency manager for Cocoa projects. To install Static with CocoaPods:

Make sure CocoaPods is installed (Static requires version 0.37 or greater).

Update your Podfile to include the following:

```ruby
use_frameworks!
pod 'Static', git: 'https://github.com/venmo/Static'
```

Run `pod install`.


### Manual

For manual installation, it's recommended to add the project as a subproject to your project or workspace and adding the appropriate framework as a target dependency.


## Usage

An [example app](Example) is included demonstrating Static's functionality.


### Getting Started

To use Static, you need to define [`Row`s](Static/Row.swift) and [`Section`s](Static/Section.swift) to describe your data. Here's a simple example:

```swift
import Static

Section(rows: [
    Row(text: "Hello")
])
```

You can configure `Section`s and `Row`s for anything you want. Here's another example:

```swift
Section(header: "Money", rows: [
    Row(text: "Balance", detailText: "$12.00", accessory: .disclosureIndicator, selection: {
        // Show statement
    }),
    Row(text: "Transfer to Bank…", cellClass: ButtonCell.self, selection: {
        [unowned self] in
        let viewController = ViewController()
        self.presentViewController(viewController, animated: true, completion: nil)
    })
], footer: "Transfers usually arrive within 1-3 business days.")
```

Since this is Swift, we can provide instance methods instead of inline blocks for selections. This makes things really nice. You don't have to switch on index paths in a `tableView:didSelectRowAtIndexPath:` any more!


### Customizing Appearance

The `Row` never has access to the cell. This is by design. The `Row` shouldn't care about its appearance other than specifying what will handle it. In practice, this has been really nice. Our cells have one responsibility.

There are several custom cells provided:

* `Value1Cell` — This is the default cell. It's a plain `UITableViewCell` with the `.Value1` style.
* `Value2Cell` — Plain `UITableViewCell` with the `.Value2` style.
* `SubtitleCell` — Plain `UITableViewCell` with the `.Subtitle` style.
* `ButtonCell` — Plain `UITableViewCell` with the `.Default` style. The `textLabel`'s `textColor` is set to the cell's `tintColor`.

All of these conform to [`Cell`](Static/Cell.swift). The gist of the protocol is one method:

```swift
func configure(row row: Row)
```

This gets called by [`DataSource`](Static/DataSource.swift) (which we'll look at more in a minute) to set the row on the cell. There is a default implementation provided by the protocol that simply sets the `Row`'s `text` on the cell's `textLabel`, etc. If you need to do custom things, this is a great place to hook in.

`Row` also has a `context` property. You can put whatever you want in here that the cell needs to know. You should try to use this as sparingly as possible.


### Custom Row Accessories

`Row` has an `accessory` property that is an `Accessory` enum. This has cases for all of `UITableViewCellAccessoryType`. Here's a row with a checkmark:

```swift
Row(text: "Buy milk", accessory: .checkmark)
```

Easy enough. Some of the system accessory types are selectable (like that little *i* button with a circle around it). You can make those and handle the selection like this:

```swift
Row(text: "Sam Soffes", accessory: .detailButton({
  // Show info about this contact
}))
```

Again, you could use whatever function here. Instance methods are great for this.

There is an additional case called `.view` that takes a custom view. Here's a `Row` with a custom accessory view:

```swift
Row(text: "My Profile", accessory: .view(someEditButton))
```


### Custom Section Header & Footer Views

`Section` has properties for `header` and `footer`. These take a `Section.Extremity`. This is an enum with `Title` and `View` cases. `Extremity` is `StringLiteralConvertible` you can simply specify strings if you want titles like we did the [Getting Started](#getting-started) section.

For a custom view, you can simply specify the `View` case:

```swift
Section(header: .view(yourView))
```

The height returned to the table view will be the view's `bounds.height` so be sure it's already sized properly.


### Working with the Data Source

To hook up your `Section`s and `Row`s to a table view, simply initialize a `DataSource`:

```swift
let dataSource = DataSource()
dataSource.sections = [
    Section(rows: [
        Row(text: "Hello")
    ])
]
```

Now assign your table view:

```swift
dataSource.tableView = tableView
```

Easy as that! If you modify your data source later, it will automatically update the table view for you. It is important that you don't change the table view's `dataSource` or `delegate`. The `DataSource` needs to be those so it can handle events correctly. The purpose of `Static` is to abstract all of that away from you.


### Wrapping Up

There is a provided [`TableViewController`](Static/TableViewController.swift) that sets up a `DataSource` for you. Here's a short example:

```swift
class SomeViewController: TableViewController {
    override func viewDidLoad() {
        super.viewDidLoad()
        dataSource.sections = [
            Section(rows: [
                Row(text: "Hi")
            ]),
            // ...
        ]
    }
}
```

Enjoy.
