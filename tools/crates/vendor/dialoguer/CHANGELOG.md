# Changelog

## 0.10.4

### Enhancements

* Added validator for password input

## 0.10.3

### Enhancements

* Fix various issues with fuzzy select
* Enable customization of number of rows for fuzzy select
* Added post completion text for input
* Various cursor movement improvements
* Correctly ignore unknown keys.
* Reset prompt height in `TermThemeRenderer::clear`.

## 0.10.2

### Enhancements

* Fix fuzzy select active item colors.
* Fix fuzzy search clear on cancel.
* Clear everything on cancel via escape key.

## 0.10.1

### Enhancements

* Allow matches highlighting for `FuzzySelect`

## 0.10.0

### Enhancements

* Loosen some trait bounds
* Improve keyboard interactions (#141, #162)
* Added `max_length` to `MultiSelect`, `Select` and `Sort`
* Allow completion support for `Input::interact_text*` behind `completion` feature

### Breaking

* All prompts `*::new` will now don't report selected values unless `report(true)` is called on them.

## 0.9.0

### Enhancements

* Apply input validation to the default value too in `Input`
* Added `FuzzySelect` behind `fuzzy-select` feature
* Allow history processing for `Input::interact_text*` behind `history` feature
* Added `interact_*_opt` methods for `MultiSelect` and `Sort`.

### Breaking

* Updated MSRV to `1.51.0`
* `Editor` is gated behind `editor` feature
* `Password`, `Theme::format_password_prompt` and `Theme::format_password_prompt_selection` are gated behind `password` feature
* Remove `Select::paged()`, `Sort::paged()` and `MultiSelect::paged()` in favor of automatic paging based on terminal size

## 0.8.0

### Enhancements

* `Input::validate_with` can take a `FnMut` (allowing multiple references)

### Breaking

* `Input::interact*` methods take `&mut self` instead of `&self`

## 0.7.0

### Enhancements

* Added `wait_for_newline` to `Confirm`
* More secure password prompt
* More documentation
* Added `interact_text` method for `Input` prompt
* Added `inline_selections` to `ColorfulTheme`

### Breaking

* Removed `theme::CustomPromptCharacterTheme`
* `Input` validators now take the input type `T` as arg
* `Confirm` has no `default` value by default now

## 0.6.2

### Enhancements

* Updating some docs

## 0.6.1

### Bugfixes

* `theme::ColorfulTheme` default styles are for stderr

## 0.6.0

### Breaking

* Removed `theme::SelectionStyle` enum
* Allowed more customization for `theme::Theme` trait by changing methods
* Allowed more customization for `theme::ColorfulTheme` by changing members
* Renamed prompt `Confirmation` to `Confirm`
* Renamed prompt `PasswordInput` to `Password`
* Renamed prompt `OrderList` to `Sort`
* Renamed prompt `Checkboxes` to `MultiSelect`

### Enhancements

* Improved colored theme
* Improved cursor visibility manipulation
