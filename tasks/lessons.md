# Lessons

- The `brave-browser` repository contains issues, releases, and wiki content; iOS source work belongs in `brave-core`.
- The iOS simulator native build requires `--target_environment=simulator` in addition to the iOS target OS and architecture.
- This Brave revision requires Swift package tools 6.2.1: Xcode 16.4 supplies the legacy `intentbuilderc` utility but cannot resolve the package; Xcode 26.6 resolves Swift 6.2 packages but lacks `intentbuilderc`.
- For the iPad tab strip, a 40 pt bar, 44 pt close hit area, and 20 pt `xmark.circle.fill` provide a Safari-style control that adapts cleanly to light and dark mode.
- Keep navigation UI discoverable when its backing folder is empty: show the Mobile Bookmarks root entry rather than hiding the entire bookmark bar.
- `BookmarkManager.waitForBookmarkModelLoaded` has one pending observer; work that needs the loaded model should share the same callback rather than register a competing wait.
- For compact horizontal toolbar buttons, use `UIStackView.Distribution.fillProportionally`; plain fill can assign every button the widest item's width.
