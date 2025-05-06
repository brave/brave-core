### Keyed Services

Most features should be scoped to a Profile/BrowserContext and be BrowserContextKeyedServices per https://www.chromium.org/developers/design-documents/profile-architecture/. What this means is that all associated preferences and other locally stored state will be per-profile. A service may sometimes be shared between the regular and OTR profile (check with sec-team) and it may also be ununavailable for certain profile types. `ProfileKeyedServiceFactory/ProfileKeyedServiceFactoryIOS` with `ProfileSelections::Builder` is the preferred method to use for determining which (if any) profile should be used for the service.

```
MyServiceFactory::MyServiceFactory()
    : ProfileKeyedServiceFactory(
          "MyService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) { ... }
```

If you have more complicated logic then use `BrowserContextKeyedServiceFactory::GetBrowserContextToUse` and return nullptr if the service is not available for the given `BrowserContext`.

```
content::BrowserContext* MyServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsRegularProfile() &&
         base::FeatureList::IsEnabled(features::kMyFeature);
}
```

Do not explicitly return null from other methods like `BuildServiceInstanceForBrowserContext`, `GetForProfile`, etc... If you return nullptr from `GetBrowserContextToUse` or configure `ProfileSelections`, the service will already be null. Avoid using helper methods like `IsAllowedForProfile` to see if a service is available for a particular profile as these are often used incorrectly/inconsistently. The preferred method is just a null check on the service itself. Remember to prefer dependency injection https://chromium.googlesource.com/chromium/src/+/main/docs/chrome_browser_design_principles.md#structure_modularity and pass in the services you require instead of calling the factory methods internally. This reduces dependencies on the factories and makes unit testing/mocking simpler.

#### Keyed Services and Mojo

Avoid adding wrapper classes around keyed services to implement mojo interfaces. It's usually better to implement them directly in the service. Factories should implement `GetRemoteForProfile` to return a `mojo::PendingRemote` and `BindRemoteForProfile` when you are passing in a `mojo::PendingReceiver`. The keyed service should implement `MakeRemote()` to return the `mojo::PendingRemote`. Please always ensure that you check to make sure the service is not null before calling `MakeRemote()` and return an empty pending remote if it is `mojo::PendingRemote<mojom::MyService>()`.

Java code should should pass the Mojo remote using JNI.

```
static jlong JNI_MyService_GetForProfile(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& profile_android) {
  auto* profile = Profile::FromJavaObject(profile_android);
  auto pending =
      MyServiceFactory::GetInstance()->GetRemoteService(profile);
  return pending.PassPipe().release().value()
}
```

```
public @Nullable MyService getForProfile(Profile profile,
        @Nullable ConnectionErrorHandler connectionErrorHandler) {
    long nativeHandle = MyServiceFactoryJni.get().getForProfile(profile);
    MessagePipeHandle handle =
            CoreImpl.getInstance().acquireNativeHandle(nativeHandle).
            toMessagePipeHandle();
    if (!handle.isValid()) {
      return null;
    }
    MyService myService = MyService.MANAGER.attachProxy(handle, 0);
    if (connectionErrorHandler != null) {
        Handler handler = ((Interface.Proxy) myService).getProxyHandler();
        handler.setErrorHandler(connectionErrorHandler);
    }
    return myService;
}
```
