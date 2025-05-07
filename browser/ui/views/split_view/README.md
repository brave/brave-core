# Split view

Browser uses split view feature to shows two different tabs' contents at once.

brave/browser/ui/views/split_view has Ui implementation of SplitView feature.
It runs behind the tabs::features::kBraveSplitView feature.

SplitView is container view that holds two contents contents views and
SplitViewLayoutManager manages these two contents views' layout.

The two tabs to be shown in SplitView are provided by the TabTileModel.
TabTile is consists of two adjacent tabs and each tab's contents are shown together
in SplitView.

For now, TabTile is the only data source for SplitView.
