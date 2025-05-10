# Split view

Browser uses split view feature to show two different tabs' contents at once.

This feature is behind the tabs::features::kBraveSplitView.

SplitView(brave/browser/ui/views/split_view/split_view.h) has UI implementation that
shows two tabs' contents together.
SplitView is container view that holds two contents contents views and
SplitViewLayoutManager manages these two contents views' layout.
SplitViewLocationBar is mini url bar that shows split view's inactive(secondary) contents url.

In tab strip UI, we have TabTile(brave/browser/ui/tabs/tab_tile_model.h).
TabTile represents two tabs tied together like tile in tab strip UI.
When two tabs are tiled, we made both tabs are adjacent in tab strip and we use split view to show
two tabs in a tile together.

For now, TabTile is the only data source for SplitView.
And we're planning to use SplitView for showing sidebar's web panel.
