# iOS Best Practices

<a id="IOS-001"></a>

## ✅ Use `[weak self]` in Async Closures to Prevent Retain Cycles

**Always capture `[weak self]` in closures that outlive the current scope** — network callbacks, notification handlers, animation completions, and stored closures. Use `[unowned self]` only when the closure's lifetime is guaranteed to be shorter than `self`.

**Exception:** Fire-and-forget `Task {}` blocks that are **not** stored as a property do not need `[weak self]` because they don't create a retain cycle.

```swift
// ❌ WRONG - strong self capture in stored closure
dataSource.onUpdate = {
  self.reloadData()  // Retain cycle!
}

// ✅ CORRECT - weak self in stored closure
dataSource.onUpdate = { [weak self] in
  self?.reloadData()
}

// ✅ CORRECT - fire-and-forget Task (no retain cycle)
Task {
  await self.fetchData()
}

// ❌ WRONG - Task stored as property without weak self
fetchTask = Task {
  await self.fetchData()  // Retain cycle!
}

// ✅ CORRECT - weak self when Task is stored
fetchTask = Task { [weak self] in
  await self?.fetchData()
}
```

**Apply `[weak self]` to the root/outermost closure**, not inner closures. Inner closures inherit the capture from the outer scope.

---

<a id="IOS-002"></a>

## ✅ Use `@Observable` Macro, Not `ObservableObject`

**Prefer the `@Observable` macro (Observation framework) over `ObservableObject` (Combine).** `@Observable` is the modern replacement, requires less boilerplate, and integrates better with SwiftUI.

```swift
// ❌ WRONG - legacy Combine pattern
class ViewModel: ObservableObject {
  @Published var items: [Item] = []
}

// ✅ CORRECT - modern Observation framework
@Observable
class ViewModel {
  var items: [Item] = []
}
```

---

<a id="IOS-003"></a>

## ✅ Use `@MainActor` to Ensure UI Work Runs on the Main Thread

**Mark methods and types that perform UI work as `@MainActor`** so that async contexts are forced to await them. Note that Tasks inherit actor isolation from their enclosing context — adding `@MainActor` to a Task created inside an already-`@MainActor`-isolated context is redundant.

```swift
// ❌ WRONG - redundant @MainActor on Task inside @MainActor context
@MainActor
class MyViewController: UIViewController {
  func refresh() {
    Task { @MainActor in  // Redundant — Task inherits MainActor from class
      tableView.reloadData()
    }
  }
}

// ✅ CORRECT - Task inherits @MainActor from enclosing class
@MainActor
class MyViewController: UIViewController {
  func refresh() {
    Task {
      let data = await fetchData()
      tableView.reloadData()  // Already on MainActor
    }
  }
}

// ✅ CORRECT - mark the method as @MainActor when not in an isolated context
func updateUI() async {
  let data = await fetchData()
  await tableView.reloadData()  // UITableView is NS_SWIFT_UI_ACTOR, requires await
}

@MainActor
func updateUI() async {
  let data = await fetchData()
  tableView.reloadData()  // No await needed — method is MainActor-isolated
}
```

---

<a id="IOS-004"></a>

## ✅ Use `#available` Compiler Checks

**Use `#available` for API availability checks, not manual version comparisons.** The compiler enforces `#available` correctness and produces warnings when minimum deployment targets change.

```swift
// ❌ WRONG - manual version check
if ProcessInfo.processInfo.operatingSystemVersion.majorVersion >= 17 {
  useNewAPI()
}

// ✅ CORRECT - compiler-enforced availability
if #available(iOS 17, *) {
  useNewAPI()
}
```

---

<a id="IOS-005"></a>

## ✅ Prefer Swift Idioms for Concise Code

**Use Swift-native patterns instead of verbose alternatives:**

```swift
// Use expression-level switch
let title = switch state {
  case .loading: "Loading..."
  case .loaded: "Done"
  case .error: "Failed"
}

// Use for-where instead of guard/continue
// ❌ WRONG
for item in items {
  guard item.isEnabled else { continue }
  process(item)
}
// ✅ CORRECT
for item in items where item.isEnabled {
  process(item)
}

// Use max(by:)/min(by:) instead of sorted().first
// ❌ WRONG
let newest = items.sorted(by: { $0.date > $1.date }).first
// ✅ CORRECT
let newest = items.max(by: { $0.date < $1.date })

// Combine guard conditions
// ❌ WRONG
guard let a = optionalA else { return }
guard let b = optionalB else { return }
// ✅ CORRECT
guard let a = optionalA, let b = optionalB else { return }
```

---

<a id="IOS-006"></a>

## ✅ Use `Set` for Membership Testing

**Use `Set` instead of `Array` when the primary operation is membership testing (`contains`).** `Set.contains` is O(1) vs Array's O(n).

```swift
// ❌ WRONG - O(n) lookup
let allowedTypes: [String] = ["image", "video", "audio"]
if allowedTypes.contains(type) { ... }

// ✅ CORRECT - O(1) lookup
let allowedTypes: Set<String> = ["image", "video", "audio"]
if allowedTypes.contains(type) { ... }
```

---

<a id="IOS-007"></a>

## ✅ Use `public` Not `open` Unless Subclassing Is Required

**Default to `public` access control. Only use `open` when external modules need to subclass or override.** Using `open` unnecessarily exposes implementation details.

---

<a id="IOS-008"></a>

## ✅ Prefer Convenience Initializers on Enums Over Static Factory Methods

**Use convenience initializers on enums instead of static factory methods.** This is more idiomatic Swift and integrates better with the type system.

```swift
// ❌ WRONG - static factory
extension TabType {
  static func from(isPrivate: Bool) -> TabType {
    isPrivate ? .private : .regular
  }
}

// ✅ CORRECT - convenience init
extension TabType {
  init(isPrivate: Bool) {
    self = isPrivate ? .private : .regular
  }
}
```

---

<a id="IOS-009"></a>

## ❌ No Default Values for Required Init Parameters

**Don't provide default values for parameters that callers should always explicitly specify.** This prevents accidental use of incorrect defaults.

---

<a id="IOS-010"></a>

## ✅ Use `let` Over `var` for Single-Assignment Properties

**Use `let` for properties that are assigned once and never mutated.** The compiler enforces immutability, preventing accidental reassignment.

**Exception:** Swift structs control mutability at the usage site via `let`/`var`, so `var` properties in structs are acceptable:

```swift
struct Item {
  var name: String  // OK — mutability controlled by usage
}

let itemOne = Item(name: "...") // itemOne.name is immutable
var itemTwo = Item(name: "...") // itemTwo.name is mutable
```

---

<a id="IOS-011"></a>

## ❌ Avoid Implicitly Unwrapped Optionals for viewDidLoad Properties

**Do not use implicitly unwrapped optionals (`!`) for properties initialized in `viewDidLoad`.** Use lazy properties, optional types with safe unwrapping, or initialize in the initializer.

**Use `lazy var` only when** you need to defer creation to avoid work at init time, or when the initializer requires `self`.

```swift
// ❌ WRONG - crash risk if accessed before viewDidLoad
var tableView: UITableView!

// ✅ CORRECT - lazy initialization (defers creation, can reference self)
lazy var tableView = UITableView()
```

---

<a id="IOS-012"></a>

## ✅ Use `withTaskCancellationHandler` When Bridging Callbacks to Async

**When wrapping callback-based APIs with `withCheckedContinuation`, use `withTaskCancellationHandler` to propagate cancellation properly.**

```swift
// ✅ CORRECT - cancellation-aware bridging
func fetchData() async throws -> Data {
  try await withTaskCancellationHandler {
    try await withCheckedThrowingContinuation { continuation in
      request.start { result in
        continuation.resume(with: result)
      }
    }
  } onCancel: {
    request.cancel()
  }
}
```

---

<a id="IOS-013"></a>

## ✅ Always Call Completion Handlers on All Code Paths

**When a function accepts a completion handler, always call it on every code path — including error and early-return paths.** Failing to call the completion handler leaks resources and may leave callers waiting indefinitely.

```swift
// ❌ WRONG - completion handler not called on error path
func fetchData(from url: URL, _ completionHandler: @escaping () -> Void) {
  if url.scheme != "https" { return } // Didn't call completion handler on failure case
  // ...
}

// ✅ CORRECT - completion handler called in all paths
func fetchData(from url: URL, _ completionHandler: @escaping () -> Void) {
  if url.scheme != "https" {
    completionHandler()
    return
  }
  // ...
}
```

---

<a id="IOS-014"></a>

## ✅ Use `evaluateSafeJavaScript` Instead of Raw JS Evaluation

**Use the `evaluateSafeJavaScript` wrapper instead of calling `callAsyncJavaScript` or `evaluateJavaScript` directly.** The wrapper provides proper error handling and security checks.

---

<a id="IOS-015"></a>

## ✅ Model Error States as Swift `Error` Types

**Use Swift `Error` enums/structs for error representation, not integer error codes.** This provides type safety, exhaustive switch handling, and better debugging.

```swift
// ❌ WRONG - integer error codes
enum FetchError: Int {
  case networkError = 1
  case parseError = 2
}

// ✅ CORRECT - proper Error type
enum FetchError: Error {
  case networkFailure(URLError)
  case parseFailure(DecodingError)
}
```

---

<a id="IOS-016"></a>

## ✅ Delegate Methods Should Take Non-Optional Parameters

**Delegate method parameters should be non-optional.** The delegate should not need to handle nil values from the delegating object.

---

<a id="IOS-017"></a>

## ✅ Use `_ = variable` to Silence "Wrote But Not Read" Warnings

**When a variable is intentionally unused (e.g., the result of a side-effecting function), assign it to `_` explicitly.** This signals intent to future readers and silences compiler warnings.

---

<a id="IOS-018"></a>

## ✅ Use `.clipShape` Instead of Deprecated `.cornerRadius()`

**Use `.clipShape(.rect(cornerRadius:))` instead of the deprecated `.cornerRadius()` modifier.**

```swift
// ❌ WRONG - deprecated modifier
Text("Hello")
  .cornerRadius(8)

// ✅ CORRECT - modern API
Text("Hello")
  .clipShape(.rect(cornerRadius: 8))
```

---

<a id="IOS-019"></a>

## ✅ Use `.background(_:in:)` With Shape Parameter

**Use the `.background(_:in:)` modifier with a shape parameter instead of layering `.background()` with `.clipShape()`.**

```swift
// ❌ WRONG - separate background and clip
Text("Hello")
  .background(Color.blue)
  .clipShape(RoundedRectangle(cornerRadius: 8))

// ✅ CORRECT - combined
Text("Hello")
  .background(.blue, in: .rect(cornerRadius: 8))
```

---

<a id="IOS-020"></a>

## ✅ Use Stack Spacing Instead of Individual Padding

**Use the `spacing` parameter on `VStack`/`HStack` instead of adding padding to each child element.** This is cleaner and more maintainable.

```swift
// ❌ WRONG - padding on each child
VStack {
  Text("Title")
    .padding(.bottom, 8)
  Text("Subtitle")
    .padding(.bottom, 8)
  Text("Body")
}

// ✅ CORRECT - stack spacing
VStack(spacing: 8) {
  Text("Title")
  Text("Subtitle")
  Text("Body")
}
```

---

<a id="IOS-021"></a>

## ✅ Extract Inline SwiftUI Blocks Into Named Views

**When an inline SwiftUI view builder block exceeds a few lines, extract it into a named `View` struct or computed property.** This improves readability and enables reuse.

---

<a id="IOS-022"></a>

## ✅ Use `UIHostingConfiguration` for SwiftUI in UITableViewCell

**When using SwiftUI views inside `UITableViewCell` or `UICollectionViewCell`, use `UIHostingConfiguration` instead of manually embedding a `UIHostingController`.**

```swift
// ✅ CORRECT - modern cell configuration
cell.contentConfiguration = UIHostingConfiguration {
  MySwiftUIView(item: item)
}
```

---

<a id="IOS-023"></a>

## ❌ Never Modify Constraints in `layoutSubviews`

**Do not create or modify Auto Layout constraints inside `layoutSubviews`.** This causes infinite layout loops. Use `updateViewConstraints` or set constraints once during setup.

```swift
// ❌ WRONG - constraints in layoutSubviews (infinite loop risk)
override func layoutSubviews() {
  super.layoutSubviews()
  widthConstraint.constant = bounds.width / 2
}

// ✅ CORRECT - use updateViewConstraints
override func updateViewConstraints() {
  widthConstraint.constant = bounds.width / 2
  super.updateViewConstraints()
}
```

---

<a id="IOS-024"></a>

## ✅ Use `setNeedsUpdateConstraints()` for Constraint Changes

**When constraints need to change, call `setNeedsUpdateConstraints()`, not `setNeedsLayout()` + `layoutIfNeeded()`.** The constraint system handles layout invalidation.

---

<a id="IOS-025"></a>

## ✅ Use `UIView.keyboardLayoutGuide` Instead of Manual Keyboard Height

**Use `UIView.keyboardLayoutGuide` for keyboard-responsive layouts instead of manually observing keyboard notifications and adjusting insets.**

```swift
// ❌ WRONG - manual keyboard observation
NotificationCenter.default.addObserver(self,
  selector: #selector(keyboardWillShow),
  name: UIResponder.keyboardWillShowNotification, object: nil)

// ✅ CORRECT - keyboardLayoutGuide
bottomConstraint = view.bottomAnchor.constraint(
  equalTo: view.keyboardLayoutGuide.topAnchor)
```

---

<a id="IOS-027"></a>

## ✅ Use Brave Design System Colors

**Use `UIColor(braveSystemName:)` for colors, not hard-coded values or system colors.** This ensures consistency with the Brave design system and proper dark mode support.

**Avoid deprecated legacy colors** such as `UIColor.braveBlurpleTint` / `Color(.braveBlurpleTint)` from [LegacyColors.swift](https://github.com/brave/brave-core/blob/00785ba10b5dea2f7ac4e1d66dafe0ddd6d1c0af/ios/brave-ios/Sources/DesignSystem/Colors/LegacyColors.swift). The only legacy colors still acceptable in the short-term are the "grouped" backgrounds: `braveGroupedBackground`, `secondaryBraveGroupedBackground`, `tertiaryBraveGroupedBackground`.

```swift
// ❌ WRONG - hard-coded color
view.backgroundColor = UIColor(red: 0.2, green: 0.4, blue: 0.8, alpha: 1)

// ❌ WRONG - deprecated legacy color
view.tintColor = UIColor.braveBlurpleTint

// ✅ CORRECT - design system color
view.backgroundColor = UIColor(braveSystemName: .primary20)
```

---

<a id="IOS-028"></a>

## ✅ Keep Tab Observer and Policy Decider Thin

**Do not add feature-specific business logic directly in `BVC+TabObserver.swift` or `BVC+TabPolicyDecider.swift`.** These files should only contain thin delegation. Feature-specific logic belongs in `Tab` helpers.

---

<a id="IOS-029"></a>

## ✅ Set Properties Before Dependent Setup in `viewDidLoad`

**In `viewDidLoad`, set properties before calling setup methods that depend on them.** Ordering bugs here are common and hard to debug.

```swift
// ❌ WRONG - setup uses property before it's set
override func viewDidLoad() {
  super.viewDidLoad()
  setupTableView()       // Uses dataSource, which isn't set yet!
  dataSource = makeDataSource()
}

// ✅ CORRECT - property set first
override func viewDidLoad() {
  super.viewDidLoad()
  dataSource = makeDataSource()
  setupTableView()
}
```

---

<a id="IOS-030"></a>

## ✅ Wrap ObjC Headers With Nullability Annotations

**All Objective-C headers should use `NS_ASSUME_NONNULL_BEGIN`/`NS_ASSUME_NONNULL_END` and explicitly annotate nullable parameters.**

```objc
// ✅ CORRECT
NS_ASSUME_NONNULL_BEGIN

@interface BraveManager : NSObject
- (void)loadURL:(NSURL *)url completion:(nullable CompletionBlock)completion;
@end

NS_ASSUME_NONNULL_END
```

---

<a id="IOS-031"></a>

## ✅ Mark Static-Only ObjC Classes With `NS_UNAVAILABLE` Init

**For Objective-C classes that should not be instantiated (only class methods), mark `init` as `NS_UNAVAILABLE`.** Marking `new` is not necessary since most Objective-C usage is from Swift, which does not call `new`.

```objc
@interface BraveUtils : NSObject
+ (NSString *)userAgent;
- (instancetype)init NS_UNAVAILABLE;
@end
```

---

<a id="IOS-032"></a>

## ✅ `OBJC_EXPORT` Goes on Headers Only

**`OBJC_EXPORT` should only appear on the header declaration, not the implementation.**

---

<a id="IOS-033"></a>

## ✅ Use `NS_SWIFT_NAME` to Control Swift Method Naming

**Use `NS_SWIFT_NAME` on Objective-C APIs to provide idiomatic Swift names.** This is especially important for factory methods and methods with C-style naming.

---

<a id="IOS-034"></a>

## ✅ Use `isEqualToString:` for NSString Comparison

**Never use `==` or `!=` for `NSString` comparison — use `isEqualToString:`.** The `==` operator compares pointer identity, not string contents.

```objc
// ❌ WRONG - pointer comparison
if (string == @"expected") { ... }

// ✅ CORRECT - value comparison
if ([string isEqualToString:@"expected"]) { ... }
```

---

<a id="IOS-035"></a>

## ✅ Use camelCase for Obj-C Variable Names Inside Obj-C Classes/Types

**Objective-C variable naming must use camelCase inside Obj-C classes and types.** When Obj-C types are used inside C++ classes, follow C++ snake_case conventions instead.

---

<a id="IOS-037"></a>

## ✅ Weak/Strong Dance in Posted Tasks

**When posting tasks from Objective-C++ that capture `self`, pass `weakSelf` as a closure parameter rather than capturing it directly.**

```objc
// ❌ WRONG - capturing weakSelf directly in block
__weak typeof(self) weakSelf = self;
dispatch_async(queue, ^{
  [weakSelf doWork];  // weakSelf may be nil after check
});

// ✅ CORRECT - weak/strong dance
__weak typeof(self) weakSelf = self;
dispatch_async(queue, ^{
  __strong typeof(weakSelf) strongSelf = weakSelf;
  if (!strongSelf) return;
  [strongSelf doWork];
});
```

---

<a id="IOS-038"></a>

## ✅ Ensure ObjC Collection Mutability Correctness

**Use `copy` for immutable and `mutableCopy` for mutable collections.** Passing a mutable collection where an immutable one is expected (or vice versa) leads to subtle bugs.

---

<a id="IOS-039"></a>

## ❌ No Singletons — Use Dependency Injection

**Do not use singletons for service access.** Pass dependencies through initializers, method parameters, or the `Tab`/`TabHelper` extension pattern.

```swift
// ❌ WRONG - singleton access
let controller = BraveRewardsViewController.shared
controller.fetchBalance()

// ✅ CORRECT - dependency injection
init(rewardsController: BraveRewardsViewController) {
  self.rewardsController = rewardsController
}
```

---

<a id="IOS-040"></a>

## ✅ Tab Helpers Must Use `TabDataValues` Extension Pattern

**Tab-associated data should use the `TabDataValues` extension pattern, not stored properties or associated objects.** This ensures proper lifecycle management.

---

<a id="IOS-041"></a>

## ❌ Do Not Add Feature Data to `TabState`

**Do not add feature-related data directly to `TabState`.** Instead, use a Tab Helper with `TabDataValues` to store feature-specific state. `TabState` should remain a minimal model representing core tab state.

---

<a id="IOS-042"></a>

## ❌ View Model Types Must Not Import SDK Singletons

**View models must not import or reference SDK singletons directly.** Pass SDK dependencies through the initializer for testability.

---

<a id="IOS-043"></a>

## ✅ Bind ProfileIOS Directly, Avoid `getLastUsedProfile`

**Pass `ProfileIOS` explicitly into closures and initializers.** Do not call `getLastUsedProfile` at point of use — the last-used profile can change.

---

<a id="IOS-044"></a>

## ✅ Feature Flag Checks Gate Registration, Not Runtime

**Feature flag checks should gate handler/observer registration, not runtime checks inside the handler.** This avoids unnecessary overhead when the feature is disabled.

```swift
// ❌ WRONG - checking flag at runtime inside handler
func handle(event: Event) {
  guard FeatureFlags.isEnabled(.myFeature) else { return }
  // handle event...
}

// ✅ CORRECT - only register handler when feature is enabled
if FeatureFlags.isEnabled(.myFeature) {
  eventBus.register(handler: MyFeatureHandler())
}
```

---

<a id="IOS-045"></a>

## ❌ Feature-Specific Logic Does Not Belong in Generic Extensions

**Do not add feature-specific logic to generic utility extensions.** Create feature-specific helpers instead.

---

<a id="IOS-046"></a>

## ✅ Register Prefs in `browser_prefs.mm` on iOS

**iOS preference registration goes in `browser_prefs.mm`, not in arbitrary init methods.**

---

<a id="IOS-047"></a>

## ✅ Use `P3A.installationDate`, Not `DAU.installationDate`

**`Preferences.DAU.installationDate` resets after 14 days. Use `Preferences.P3A.installationDate` for persistent installation dates.**

---

<a id="IOS-048"></a>

## ✅ Wire Complete Observer/Bridge Chains

**When connecting C++ observers to Objective-C/Swift code, ensure the entire chain is wired: C++ observer → bridge object → Obj-C/Swift delegate.** Missing links in the chain are a common source of silent failures.

---

<a id="IOS-049"></a>

## ✅ Use `raw_ptr<T>` for Stored Pointers in C++ Classes

**When a C++ class stores a pointer to another object, use `raw_ptr<T>` instead of raw `T*`.** This enables Chromium's pointer safety checks.

---

<a id="IOS-050"></a>

## ✅ Use `NOTIMPLEMENTED()` for Potentially Reachable Code

**`NOTREACHED()` crashes in release builds. Use `NOTIMPLEMENTED()` for code paths that may be reached but are not yet implemented.** Only use `NOTREACHED()` when the code path should truly never execute.

---

<a id="IOS-051"></a>

## ✅ Use Mojo `::Name_` Constants for Interface Registration

**When registering Mojo interfaces, use the `::Name_` constant from the generated interface, not hard-coded strings.**

---

<a id="IOS-052"></a>

## ❌ Do Not Rely on iOS Caches Directory for Persistent State

**The iOS caches directory can be cleared by the system at any time.** Do not store state that must survive app restarts in the caches directory.

---

<a id="IOS-053"></a>

## ✅ New Strings Go in `BraveStrings`, Not `SharedStrings.swift`

**Add new user-facing strings to `BraveStrings`, not `SharedStrings.swift`.** SharedStrings is for legacy/shared strings only.

---

<a id="IOS-054"></a>

## ❌ Do Not Add New Functionality to the Legacy `Shared` Module

**The `Shared` module is legacy — do not add new functionality to it.** If you need a shared module for new code, create a new module instead. Existing `Shared` code can still be used but should not be extended.

---

<a id="IOS-055"></a>

## ✅ Tests Mirror Source Modules

**Test files should be organized to mirror the source module structure.** A class in `BraveCore/Wallet/` should have tests in `BraveCoreTests/Wallet/`.

---

<a id="IOS-056"></a>

## ✅ Export Pref Constants in Feature-Specific Headers

**iOS pref constants should be exported in feature-specific headers, not in a shared `pref_names_bridge.h`.** This reduces coupling and makes dependencies explicit.

---

<a id="IOS-057"></a>

## ✅ Mojom Wrappers Go in `ios/common`, Not `ios/browser`

**Mojom wrapper types belong in `ios/common/` because they are shared between browser and renderer processes.** Placing them in `ios/browser/` creates unnecessary layering constraints.

---

<a id="IOS-058"></a>

## ✅ Use `_bridge.h` for ObjC Wrappers, `_ios.h` for iOS Implementations

**Use the `_bridge.h` suffix for plain Objective-C wrapper headers that expose C++ types to Swift.** Use `_ios.h` for iOS-specific implementations of cross-platform interfaces.

---

<a id="IOS-059"></a>

## ✅ Use `Impl` Suffix for Concrete Implementations

**Name concrete implementations with the `Impl` suffix, not a platform suffix like `_ios`.** For example: `BraveWalletServiceImpl`, not `BraveWalletServiceIOS`.

---

<a id="IOS-060"></a>

## ✅ Remove Dead Code and Debug Statements

**Remove all debug `print` statements and dead code before submitting.** Use `Logger` instead of `print` for any logging that should remain.

---

<a id="IOS-061"></a>

## ✅ Preserve Accessibility Labels

**Always set accessibility labels on interactive elements.** VoiceOver users depend on these. When refactoring UI, verify that accessibility labels are preserved.

---

<a id="IOS-063"></a>

## ✅ Use `UIImage.prepareThumbnail(of:)` for Thumbnailing

**Use `UIImage.prepareThumbnail(of:)` for generating thumbnails instead of manual `UIGraphicsImageRenderer` resizing.** The system method is optimized for memory and performance.

---

<a id="IOS-064"></a>

## ✅ Prefer CSS `@media (prefers-color-scheme)` Over JS Class Toggling

**For injected web content, use CSS `@media (prefers-color-scheme: dark)` for dark mode support instead of toggling CSS classes via JavaScript.**

---

<a id="IOS-065"></a>

## ✅ Use `@_disfavoredOverload` + `@available(iOS, obsoleted:)` for Backporting

**When backporting newer APIs, use `@_disfavoredOverload` combined with `@available(iOS, obsoleted:)` to create transparent fallbacks that automatically disappear when the deployment target is raised.**

```swift
// Backport API available in iOS 17+
@_disfavoredOverload
@available(iOS, obsoleted: 17.0)
extension View {
  func scrollBounceBehavior(_ behavior: ScrollBounceBehavior) -> some View {
    // Fallback implementation
    self
  }
}
```

---

<a id="IOS-066"></a>

## ✅ Artificial Delays Require Comments

**Any use of `Task.sleep` or similar artificial delays must include a comment explaining why the delay is necessary.** Unexplained delays are a code smell and may hide race conditions.

```swift
// ❌ WRONG - unexplained delay
try await Task.sleep(nanoseconds: 500_000_000)

// ✅ CORRECT - documented reason
// Wait for WebKit to finish layout after orientation change.
// WebKit does not provide a completion callback for this operation.
try await Task.sleep(nanoseconds: 500_000_000)
```

---

<a id="IOS-067"></a>

## ✅ Use `TimestampFormatStyle` for Duration Formatting

**Use `TimestampFormatStyle` or `Duration.formatted()` for displaying durations instead of manual string formatting.**

---

<a id="IOS-068"></a>

## ✅ Prefer Local Variables for Short-Lived Cancellables

**For Combine subscriptions that are only needed temporarily (e.g., waiting for a single value), use a local variable instead of storing the cancellable as a property.** This prevents accumulation of dead subscriptions.

---

<a id="IOS-069"></a>

## ✅ Check Delegate Before `withCheckedContinuation`

**When using `withCheckedContinuation` that depends on a delegate callback, verify the delegate is set before entering the continuation.** Otherwise the continuation may never resume, hanging the task indefinitely.

---

<a id="IOS-070"></a>

## ✅ Match Button Shapes to Nala Design System

**Buttons should use rounded rectangle shapes consistent with the Nala design system.** Do not use fully rounded (capsule) or sharp-cornered buttons unless the design specifically calls for it.

---

<a id="IOS-071"></a>

## ✅ chromium_src iOS Overrides: Prefer Subclassing

**When overriding iOS Chromium classes via chromium_src, prefer subclassing over code duplication.** Use the `#define`/`#include`/`#undef` pattern to substitute Brave subclasses.

```cpp
// chromium_src/ios/chrome/browser/some_file.mm
#define ChromeViewController BraveViewController

#include <ios/chrome/browser/some_file.mm>

#undef ChromeViewController
```

**Always add blank lines around the `#define`/`#undef`-wrapped includes for readability.**

---

<a id="IOS-074"></a>

## ✅ Use GN Build Args for Platform-Specific Feature Defaults

**Use GN build arguments (not C++ source code) to set platform-specific feature defaults.** This allows build-time configuration and reduces `#ifdef` clutter.

---

<a id="IOS-075"></a>

## ✅ Include All iOS Test Targets in `brave_all_unit_tests`

**Every new iOS test target must be included in `brave_all_unit_tests`.** Tests that are not included will never run on CI.

---

<a id="IOS-076"></a>

## ✅ Feature Flag Names Must Match C++ Feature Names

**In iOS bridge headers, feature flag names must exactly match the C++ `base::Feature` names.** Mismatches cause silent failures where the flag appears to work but doesn't actually control the feature.

---

<a id="IOS-077"></a>

## ✅ Use `contentMode: .fill` or Size Constraints for Images

**Set explicit `contentMode` or size constraints on `UIImageView` and SwiftUI `Image` views.** Without these, images may display at their natural size, causing layout issues.

```swift
// SwiftUI
Image("icon")
  .resizable()
  .frame(maxWidth: 24, maxHeight: 24)

// UIKit
imageView.contentMode = .scaleAspectFill
```

---

<a id="IOS-078"></a>

## ✅ Use `Label` for Icon-Only Buttons in SwiftUI

**Always use `Label` with `.labelStyle(.iconOnly)` for icon-only buttons.** This ensures accessibility information is provided automatically.

```swift
// ❌ WRONG - no accessibility label
Button {
  // ...
} label: {
  Image(braveSystemName: "leo.trash")
}

// ✅ CORRECT - accessible icon-only button
Button {
  // ...
} label: {
  Label("Delete", braveSystemImage: "leo.trash")
    .labelStyle(.iconOnly)
}
```

---

<a id="IOS-079"></a>

## ✅ Use Nala Design System Icons Over Apple's System Icons

**Use `Image(braveSystemName:)` with Nala (Leo) icons instead of Apple's `Image(systemName:)`.** Nala icons ensure visual consistency with the Brave design system.

```swift
// ❌ WRONG - Apple system icon
Image(systemName: "trash")

// ✅ CORRECT - Nala design system icon
Image(braveSystemName: "leo.trash")
```

---

<a id="IOS-080"></a>

## ✅ Size Nala Icons Using Fonts, Not Frame

**Nala icons are custom SF Symbols and should be treated like font glyphs, not images.** Size them with `.font()`, not `.resizable().frame()`.

```swift
// ❌ WRONG - treating icon like an image
Image(braveSystemName: "leo.trash")
  .resizable()
  .frame(width: 24, height: 24)

// ✅ CORRECT - sizing with font
Image(braveSystemName: "leo.trash")
  .font(.title3)
```

---

<a id="IOS-081"></a>

## ❌ Do Not Use Explicit Font Sizes

**UI must adhere to the user's dynamic font sizing preferences.** Use semantic font styles (`.subheadline`, `.body`, etc.). If a custom value must be used, use `@ScaledMetric` in SwiftUI or `UIFontMetrics` in UIKit.

```swift
// ❌ WRONG - fixed font size ignores user preferences
Text("Hello World")
  .font(.system(size: 15))

// ✅ CORRECT - semantic font style
Text("Hello World")
  .font(.subheadline)

// ✅ CORRECT - scaled custom value
@ScaledMetric private var customSize = 15
Text("Hello World")
  .font(.system(size: customSize))
```

---

<a id="IOS-082"></a>

## ❌ Do Not Initialize SwiftUI `@State` on `init`

**Do not assign `@State` properties in `init` using `State(wrappedValue:)`.** This can cause undefined behavior on future body computations. Pass in the initial value and assign it using an `onAppear` or `task` modifier.

```swift
// ❌ WRONG - assigning @State in init
struct ContentView: View {
  @State private var value: Int
  init(value: Int) {
    self._value = State(wrappedValue: value)
  }
}

// ✅ CORRECT - assign via onAppear
struct ContentView: View {
  var initialValue: Int
  @State private var value: Int?

  var body: some View {
    Color.black
      .onAppear {
        value = initialValue
      }
  }
}
```
