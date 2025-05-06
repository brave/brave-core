### Keyed Services

Most features should be scoped to a Profile/BrowserContext and be BrowserContextKeyedServices per https://www.chromium.org/developers/design-documents/profile-architecture/. What this means is that all associated preferences and other locally stored state will be per-profile. A service may sometimes be shared between the regular and OTR profile (check with sec-team) and it may also be ununavailable for certain profile types. `ProfileKeyedServiceFactory/ProfileKeyedServiceFactoryIOS` with `ProfileSelections::Builder` is the preferred method to use for determining which (if any) profile should be used for the service.

```c++
MyServiceFactory::MyServiceFactory()
    : ProfileKeyedServiceFactory(
          "MyService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) { ... }
```

If you have more complicated logic then use `BrowserContextKeyedServiceFactory::GetBrowserContextToUse` and return nullptr if the service is not available for the given `BrowserContext`.

```c++
content::BrowserContext* MyServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsRegularProfile() &&
         base::FeatureList::IsEnabled(features::kMyFeature);
}
```

Do not explicitly return null from other methods like `BuildServiceInstanceForBrowserContext`, `GetForProfile`, etc... If you return nullptr from `GetBrowserContextToUse` or configure `ProfileSelections`, the service will already be null. Avoid using helper methods like `IsAllowedForProfile` to see if a service is available for a particular profile as these are often used incorrectly/inconsistently. The preferred method is just a null check on the service itself. Remember to prefer dependency injection https://chromium.googlesource.com/chromium/src/+/main/docs/chrome_browser_design_principles.md#structure_modularity and pass in the services you require instead of calling the factory methods internally. This reduces dependencies on the factories and makes unit testing/mocking simpler.

#### Keyed Services and Mojo

Avoid adding wrapper classes around keyed services to implement mojo interfaces. It's usually better to implement them directly in the service. Factories should implement `GetRemoteForProfile` to return a `mojo::PendingRemote` and `BindRemoteForProfile` when you are passing in a `mojo::PendingReceiver`. The keyed service should implement `MakeRemote()` to return the `mojo::PendingRemote`. Please always ensure that you check to make sure the service is not null before calling `MakeRemote()` and return an empty pending remote if it is `mojo::PendingRemote<mojom::FakeService>()`.

C++

brave/components/fake/fake_service_impl.cc
```c++
namespace fake {
class FakeServiceImpl : public KeyedService, public mojom::FakeService {
...
}
}  // namespace fake
```

brave/browser/fake/fake_service_factory.cc
```c++
mojo::PendingRemote<mojom::FakeService> FakeServiceFactory::GetRemoteForProfile(
    content::BrowserContext* context) {
  auto* instance = GetInstance()->GetServiceForBrowserContext(context, true);
  if (!instance) {
    return mojo::PendingRemote<mojom::FakeService>();
  }
  return static_cast<fake::FakeServiceImpl*>(instance)->MakeRemote();
}

void FakeServiceFactory::BindRemoteForProfile(
    content::BrowserContext* context,
    mojo::PendingReceiver<fake::mojom::FakeService> receiver) {
  auto* service = static_cast<fake::FakeServiceImpl*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
  if (service) {
    service->Bind(std::move(receiver));
  }
}

static jlong JNI_FakeService_GetForProfile(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending =
      FakeServiceFactory::GetInstance()->GetRemoteService(profile);
  return pending.PassPipe().release().value()
}
```

Java

brave/browser/fake/android/java/src/org/chromium/brave/browser/fake/FakeServiceFactory.java
```java
public @Nullable fake::FakeService getForProfile(Profile profile,
        @Nullable ConnectionErrorHandler connectionErrorHandler) {
    long nativeHandle = FakeServiceFactoryJni.get().getForProfile(profile);
    MessagePipeHandle handle =
            CoreImpl.getInstance().acquireNativeHandle(nativeHandle).
            toMessagePipeHandle();
    if (!handle.isValid()) {
      return null;
    }
    fake::FakeService fakeService =
        fake::FakeService.MANAGER.attachProxy(handle, 0);
    if (connectionErrorHandler != null) {
        Handler handler = ((Interface.Proxy) fakeService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);
    }
    return fakeService;
}
```

#### IOS

IOS has separate factories because Profile subclasses `web::BrowserState` instead of `content::BrowserContext`, but the same principles apply to `ProfileKeyedServiceFactoryIOS`.

#### IOS keyed services and mojo

obj-c

brave/ios/browser/fake/fake_service_factory.mm
```objc
mojo::PendingRemote<fake::mojom::FakeService> FakeServiceFactory::GetForProfile(
    ProfileIOS* profile) {
  auto* service = GetInstance()->GetServiceForProfileAs<fake::FakeServiceImpl>(
      profile, true);
  if (!service) {
    return mojo::PendingRemote<fake::mojom::FakeService>();
  }
  return service->MakeRemote();
}

@implementation FakeFakeServiceFactory
+ (nullable id)getForProfile:(ProfileIOS*)profile {
  auto* fake_service =
      FakeServiceFactory::GetRemoteForProfile(profile);
  if (!fake_service) {
    return nil;
  }
  mojo::PendingRemote<fake::mojom::FakeService> pending_remote;
  fake_service->Bind(pending_remote.InitWithNewPipeAndPassReceiver());
  return [[FakeFakeServiceMojoImpl alloc]
      initWithFakeService:std::move(pending_remote)];
}
```

brave/ios/browser/fake/fake_service_factory_wrapper.h
```objc
#import <Foundation/Foundation.h>
#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol FakeFakeService;

OBJC_EXPORT
NS_SWIFT_NAME(Fake.FakeServiceFactory)
@interface FakeFakeServiceFactory
    : KeyedServiceFactoryWrapper < id <FakeFakeService>
> @end

#endif  // BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_FACTORY_WRAPPERS_H_
```

swift
```swift
FakeServiceFactory.get(privateMode: privateMode),
```
