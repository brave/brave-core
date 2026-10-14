# Gestures

## Overview

Lets the user trigger browser commands by drawing with the mouse (right-click
drag, or Alt/Option+left-click drag) or two-finger touch drag. The mapping lives
under `components/gestures/core/`, split into `directional/` (8-way stroke
gestures, e.g. `↓→`) and `custom/` (freeform traced shapes). Browser-process
glue (drag tracking, dispatch, settings UI wiring) lives under
`browser/ui/gestures/`.

## Maintainers

- Desktop dev is Terry Mancey.
- Android dev is Aleksei Seren.
- Project lead is Watson.

## Terminology

- A `GesturePath` is the list of `gfx::PointF` sampled while a draw is in
  progress.
- A closed shape is one whose start and end point land close together relative
  to its own length, e.g. a circle. An open shape, e.g. a checkmark, has a
  meaningful start and end.
- Winding direction is which way a closed shape was traced, clockwise or
  counterclockwise.
- Arc length resampling places points at equal intervals along the total
  distance traveled, rather than at a fixed number of original samples.
- A point sequence is a path reduced to a fixed number of ordered points, used
  to compare two shapes regardless of how each was originally drawn.

## How a draw is matched

A draw is tracked live as a `GesturePath` (a list of `gfx::PointF`) in
`GestureDrawSession`. The directional and custom recognizers, covered below,
each try to match the finished path against what they know. Smart gestures,
covered separately further down, are resolved first and don't go through this
cascade.

### Shape matching algorithm

Both the directional and custom recognizers compare shapes with the same
point-sequence algorithm, in `gesture_geometry_util.{h,cc}`. A path is resampled
to a fixed point count evenly spaced by arc length (`ResampleToCount`), then
normalized by translating it to the origin and scaling it so its longer
dimension spans 0.0 to 1.0, preserving its aspect ratio
(`NormalizeScaleAndTranslation`). This removes the drawn size and position of
the shape from the comparison, so only its proportions matter.

Two normalized paths are compared by the average distance between points sharing
the same index (`AverageDistance`). A closed shape (one whose start and end are
close relative to its own length) has no meaningful starting point or winding
direction, so `BestAlignedCandidate` tries every rotation of the candidate's
points, in both the original and reversed order, and keeps whichever alignment
gives the smallest average distance.

A second score, the hit score (`HitScore`), ranges from 0.0 to 1.0 and averages
a per-point score that is 1.0 for a point landing exactly on its counterpart,
falling off linearly to 0.0 at a per-point distance threshold, rather than
counting a hit as all or nothing. The custom recognizer picks the saved gesture
with the highest hit score among those within the match threshold, not the one
with the smallest average distance overall, since this lets a shape that is a
near-exact fit everywhere except a couple of distinctive points beat a shape
that is mediocre everywhere.

### Directional Gestures

As the draw progresses, `DirectionalGestureRecognizer` classifies movement into
one of 8 octant directions (`DirectionalGestureStrokeType`) once it has
sustained far enough (`kDirectionalGestureMinStrokePixels`) and stayed straight
enough (`kDirectionalGestureMinStrokeStraightness`) to commit. A finished draw
is a short sequence of these, like `↓→`, looked up directly in
`DirectionalGestureMapping`.

To let a directional read compete fairly against a custom match (see below), the
resolved stroke sequence is also turned into an idealized polyline
(`DirectionalGestureStrokesToCanonicalPath`) and compared to the actual drawn
path on the same normalized distance scale custom gestures use.

### Custom Gestures

The user can record their own freeform shape (e.g. a checkmark or a circle) and
assign actions to it. Matching ignores drawing speed, size, and placement, using
the same point-sequence algorithm described above to compare the drawn path
against each recorded gesture.

### Picking the winner

`GestureDrawSession::ResolveGestureMatch` runs the directional and custom
recognizers together. Directional wins if it matches and is at least as close as
the best custom match (within a small epsilon, so floating-point noise alone
can't flip the outcome). Otherwise custom wins if it matches. A weak directional
classification that only explains part of the shape can still lose to a strong
custom match, or to no match at all.

## Smart gestures

Smart gestures are independent of the directional/custom cascade above. A
right-click-drag or Alt/Option+left-click-drag that starts over a link, an
image, or the page's selected text resolves a single cardinal direction
(up/down/left/right) against a per-content-type action mapping
(`SmartGestureMapping`), giving priority to whatever content is under the drag's
start point over the shape-based match. See
`components/gestures/core/common/smart/`.

## Key files

- `browser/ui/gestures/session/gesture_draw_session.{h,cc}` owns a single
  in-progress draw and arbitrates directional vs. custom vs. smart-gesture
  matches.
- `browser/ui/gestures/session/gesture_config_tracker.{h,cc}` owns the
  directional mapping, the custom recognizer, and the smart gesture mapping for
  a profile.
- `browser/ui/gestures/dispatch/gesture_action_dispatcher.{h,cc}` runs the
  actions a resolved gesture maps to, including fetching the focused frame's
  selected text (via the `mojom::GestureTextSelectionExtractor` renderer
  interface).
- `components/gestures/core/common/directional/directional_gesture_util.{h,cc}`
  holds the live classification rules.
- `components/gestures/core/common/gesture_geometry_util.{h,cc}` holds the
  shared resample/normalize/distance math used by the directional and custom
  recognizers.
- `components/gestures/core/common/custom/custom_gesture_recognizer.{h,cc}`
  matches a path against the user's recorded custom gestures.
- `components/gestures/core/common/smart/` holds the per-content-type smart
  gesture mapping (`smart_gesture_mapping.{h,cc}`) and its defaults
  (`smart_gesture_defaults.cc`).
- `components/gestures/renderer/gesture_text_selection_extractor.{h,cc}` is the
  renderer-side Mojo interface implementation backing the selected-text smart
  gesture actions.
