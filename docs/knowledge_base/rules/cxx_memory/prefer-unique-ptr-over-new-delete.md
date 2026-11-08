---
name: brave/prefer-unique-ptr-over-new-delete
description:
  'Objects must not be managed with manual new/delete; use std::unique_ptr
  (std::make_unique), stack variables or member initializer lists.'
rubric: safety
severity: should_fix
---

**Rule.** Do not allocate an object with `new` into a raw owning pointer and
release it later with `delete`. Hold it in a `std::unique_ptr` created with
`std::make_unique`, keep it on the stack, or construct the member in the member
initializer list.

**Why.** A raw owning pointer leaks on every early return and leaves the
`delete` to be kept in step with every path by hand, which is how leaks, double
frees and use-after-free bugs arise. A `std::unique_ptr` or a value releases the
object exactly once, automatically.

## Examples

Bad:

```cpp
void Init() {
  predictor_ = new BandwidthSavingsPredictor(extractor);
  // ...
  delete predictor_;
}
```

Good:

```cpp
MyClass::MyClass(FeatureExtractor* extractor)
    : predictor_(std::make_unique<BandwidthSavingsPredictor>(extractor)) {}
```
