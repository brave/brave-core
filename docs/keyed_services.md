### Keyed Services

Most features should be scoped to a Profile/BrowserContext and be
ProfileKeyedServiceFactory per
[profile-architecture](https://www.chromium.org/developers/design-documents/profile-architecture/).
What this means is that all associated preferences and other locally stored
state will be per-profile. A service may sometimes be shared between the regular
and OTR profile (check with sec-team) and it may also be ununavailable for
certain profile types.
`ProfileKeyedServiceFactory/ProfileKeyedServiceFactoryIOS`
[profile_keyed_service_factory.md](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/profiles/profile_keyed_service_factory.md;bpv=0)
with `ProfileSelections::Builder` is the preferred method to use for determining
which (if any) profile should be used for the service. Think twice about how your
feature / service should behave in incognito / tor / etc and ask if not sure.


keyed-service-docs
```cpp
MyServiceFactory::MyServiceFactory()
    : ProfileKeyedServiceFactory(
          "MyService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) { ... }
```

If you have more complicated logic then use
`BrowserContextKeyedServiceFactory::GetBrowserContextToUse` and return nullptr
if the service is not available for the given `BrowserContext`. The factory
should only return nullptr for attributes that are fixed for at least the life
of the browser session (profile, feature flag, etc...) and not for things like
prefs that change during the session. For a given profile/browser session it
should either always return nullptr or never return nullptr. This also applies
to `GetForProfile`, etc... (If there is logic in `GetForProfile` it should move
to `GetBrowserContextToUse`)

```cpp
content::BrowserContext* MyServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsRegularProfile() &&
         base::FeatureList::IsEnabled(features::kMyFeature);
}
```

Do not explicitly return null from other methods like
`BuildServiceInstanceForBrowserContext`, `GetForProfile`, etc... If you return
nullptr from `GetBrowserContextToUse` or configure `ProfileSelections`, the
service will already be null. Avoid using helper methods like
`IsAllowedForProfile` to see if a service is available for a particular profile
as these are often used incorrectly/inconsistently. The preferred method is just
a null check on the service itself. Prefer dependency injection
[structure-modularity](https://chromium.googlesource.com/chromium/src/+/main/docs/chrome_browser_design_principles.md#structure_modularity)
and pass in the services you require instead of calling the factory methods
internally. This reduces dependencies on the factories and makes unit
testing/mocking simpler.

#### Keyed Services and Unit Tests

If the KeyedService needs to be created automatically with the profile, then it
should not be created during unit tests otherwise it is not possible to
use a `Profile`/`TestProfileIOS` without this service (and then the tests 
cannot be independent from the service). On iOS, This behaviour will be verified 
by a `CHECK` when creating the `ProfileKeyedServiceFactoryIOS`

On desktop/android you need to return false for 
`ServiceIsCreatedWithBrowserContext` or return false for 
`ServiceIsNULLWhileTesting`

On iOS you need to either remove `ServiceCreation::kCreateWithProfile` or add 
`TestCreation::kNoServiceForTests` and inject a test factory in tests 
that need the service to exist.

#### Keyed Services and Mojo

Avoid adding wrapper classes around keyed services to implement mojo interfaces.
It's usually better to implement them directly in the service. Factories should
implement `GetRemoteForProfile` to return a `mojo::PendingRemote` and
`BindRemoteForProfile` when you are passing in a `mojo::PendingReceiver`. The
keyed service should implement `MakeRemote()` to return the
`mojo::PendingRemote`. Please always ensure that you check to make sure the
service is not null before calling `MakeRemote()` and return an empty pending
remote if it is `mojo::PendingRemote<mojom::ExampleService>()`.

C++

brave/components/example/example_service_impl.cc
```cpp
namespace example {
class ExampleServiceImpl : public KeyedService, public mojom::ExampleService {
...
}
}  // namespace example
```

brave/browser/example/example_service_factory.cc
```cpp
mojo::PendingRemote<mojom::ExampleService> ExampleServiceFactory::GetRemoteForProfile(
    content::BrowserContext* context) {
  auto* instance = GetInstance()->GetServiceForBrowserContext(context, true);
  if (!instance) {
    return mojo::PendingRemote<mojom::ExampleService>();
  }
  return static_cast<example::ExampleServiceImpl*>(instance)->MakeRemote();
}

void ExampleServiceFactory::BindRemoteForProfile(
    content::BrowserContext* context,
    mojo::PendingReceiver<example::mojom::ExampleService> receiver) {
  auto* service = static_cast<example::ExampleServiceImpl*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
  if (service) {
    service->Bind(std::move(receiver));
  }
}

static jlong JNI_ExampleService_GetForProfile(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending =
      ExampleServiceFactory::GetInstance()->GetRemoteService(profile);
  return pending.PassPipe().release().value()
}
```

Java

brave/browser/example/android/java/src/org/chromium/brave/browser/example/ExampleServiceFactory.java
```java
public @Nullable ExampleService getForProfile(Profile profile,
        @Nullable ConnectionErrorHandler connectionErrorHandler) {
    long nativeHandle = ExampleServiceFactoryJni.get().getForProfile(profile);
    MessagePipeHandle handle =
            CoreImpl.getInstance().acquireNativeHandle(nativeHandle).
            toMessagePipeHandle();
    if (!handle.isValid()) {
      return null;
    }
    ExampleService exampleService =
        ExampleService.MANAGER.attachProxy(handle, 0);
    if (connectionErrorHandler != null) {
        Handler handler = ((Interface.Proxy) exampleService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);
    }
    return exampleService;
}
```

#### iOS

iOS has separate factories because Profile subclasses `web::BrowserState`
instead of `content::BrowserContext`, but the same principles apply to
`ProfileKeyedServiceFactoryIOS`.

#### iOS and Desktop profile checks

TODO - standardize a way to maintain consistency between
`GetBrowserContextToUse` methods on desktop and android. Possibly using some
constants like `kRegular`, `kIncognito`, `kTor` so the same code can be used to
check both `Profile` and `ProfileIOS`

#### iOS keyed services and mojo

obj-c

brave/ios/browser/example/example_service_factory.mm
```cpp
mojo::PendingRemote<example::mojom::ExampleService> ExampleServiceFactory::GetRemoteForProfile(
    ProfileIOS* profile) {
  auto* service = GetInstance()->GetServiceForProfileAs<example::ExampleServiceImpl>(
      profile, true);
  if (!service) {
    return mojo::PendingRemote<example::mojom::ExampleService>();
  }
  return service->MakeRemote();
}

@implementation ExampleServiceFactory
+ (nullable id)getForProfile:(ProfileIOS*)profile {
  auto* example_service =
      ExampleServiceFactory::GetRemoteForProfile(profile);
  if (!example_service) {
    return nil;
  }
  mojo::PendingRemote<example::mojom::ExampleService> pending_remote;
  example_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[ExampleServiceMojoImpl alloc]
      initWithExampleService:std::move(pending_remote)];
}
```

brave/ios/browser/example/example_service_factory_wrapper.h
```objc
@protocol ExampleService;

OBJC_EXPORT
NS_SWIFT_NAME(Example.ExampleServiceFactory)
@interface ExampleServiceFactory
    : KeyedServiceFactoryWrapper < id <ExampleService>
> @end
```

swift
```swift
ExampleServiceFactory.get(privateMode: privateMode),
```
