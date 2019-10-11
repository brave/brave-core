#ifndef SAFETYNET_CHECK_H_
#define SAFETYNET_CHECK_H_

#include "base/android/scoped_java_ref.h"
#include "net/base/completion_once_callback.h"

#include <jni.h>
#include <string>

namespace safetynet_check {

class SafetyNetCheckRunner;
using ClientAttestationCallback = base::OnceCallback<void(bool, const std::string&)>;

class SafetyNetCheck {
  public:
    SafetyNetCheck(SafetyNetCheckRunner* runner);
    ~SafetyNetCheck();
    // Performs client attestation, called from C++
    bool clientAttestation(const std::string& nonce, ClientAttestationCallback attest_callback);
    // Callback returns client attestation final result, called from Java
    void clientAttestationResult(JNIEnv* env, const base::android::JavaRef<jobject>& jobj, jboolean result,
                                  const base::android::JavaParamRef<jstring>& jresult_string);
  private:
    base::android::ScopedJavaGlobalRef<jobject> java_obj_;
    ClientAttestationCallback attest_callback_;
    SafetyNetCheckRunner* runner_;
};

class SafetyNetCheckRunner {
  public:
    SafetyNetCheckRunner();
    ~SafetyNetCheckRunner();
    void performSafetynetCheck(const std::string& nonce, ClientAttestationCallback attest_callback_);
    void jobFinished(SafetyNetCheck* finished_job);
  private:
    std::vector<std::unique_ptr<SafetyNetCheck>> jobs_;
};

}

#endif //SAFETYNET_CHECK_H_