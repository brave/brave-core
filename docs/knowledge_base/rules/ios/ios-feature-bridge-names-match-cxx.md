---
name: brave/ios-feature-bridge-names-match-cxx
description:
  'Feature flags exposed in iOS bridge headers have exactly the name of the C++
  base::Feature they wrap.'
rubric: correctness
severity: blocking
---

**Rule.** A feature exposed to Swift through a bridge (`FeatureList` in
`ios/browser/api/features/`) MUST have exactly the name of the C++
`base::Feature` it wraps: `kBraveFoo` wraps `foo::features::kBraveFoo`, both in
the header's property and in the implementation's accessor.

**Why.** A mismatched name compiles but can wrap the wrong feature or read
differently from the code it seems to control: the flag appears to work while
the feature does not follow it.

## Examples

Bad:

```objc
+ (Feature*)kPlaylistFakeUA {
  return [[Feature alloc] initWithFeature:&playlist::features::kPlaylist];
}
```

Good:

```objc
+ (Feature*)kPlaylistFakeUA {
  return [[Feature alloc] initWithFeature:&playlist::features::kPlaylistFakeUA];
}
```
