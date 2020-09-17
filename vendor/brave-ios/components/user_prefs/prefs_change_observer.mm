#include "brave/vendor/brave-ios/components/user_prefs/prefs_change_observer.h"
#include "base/bind.h"
#include "base/callback.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_change_registrar.h"
#include <memory>

@interface BravePrefsChangeObserver()
{
    PrefService* service;
    std::unique_ptr<PrefChangeRegistrar> registrar;
}
@end

@implementation BravePrefsChangeObserver
- (instancetype)initWithObserver:(void(^)())observer {
    if ((self = [super init])) {
        registrar = std::make_unique<PrefChangeRegistrar>();
    }
    return self;
}

- (void)dealloc {
    registrar.reset();
    service = nullptr;
}

- (void)Initialize:(PrefService *)pref_service {
    service = pref_service;
}

- (void)add:(NSString *)path observer:(void(^)(NSString *))pathObserver {
    if (registrar) {
        //Bind the Objective-C++ callback to a C++ repeating callback with implicit capture
        registrar->Add([path UTF8String], base::BindRepeating([](void(^observer)(NSString*), const std::string &path) {
            observer([NSString stringWithUTF8String:path.c_str()]);
        }, pathObserver));
    }
}
        
/*- (void)add:(NSString *)path observer:(void(^)(NSString *))nameObserver {
    if (_registrar) {
        _registrar->Add([path utf8String], [pathObserver](const std::string& path){
            pathObserver([NSString stringWithUTF8String:path.c_str()];
        });
    }
}*/
                        
- (void)remove:(NSString *)path {
    if (registrar) {
        registrar->Remove([path UTF8String]);
    }
}
    
- (void)removeAll {
    if (registrar) {
        registrar->RemoveAll();
    }
}
                        
- (bool)isEmpty {
    return registrar ? registrar->IsEmpty() : false;
}
                        
//TODO: Add IsObserved, IsManaged, prefs.
@end
