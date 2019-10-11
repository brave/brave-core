 #include "safetynet_check.h"
 #include "base/android/jni_android.h"
 #include "base/android/jni_string.h"
 #include "brave/build/android/jni_headers/SafetyNetCheck_jni.h"

namespace safetynet_check {

SafetyNetCheck::SafetyNetCheck(SafetyNetCheckRunner* runner) {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_obj_.Reset(env, Java_SafetyNetCheck_create(env, reinterpret_cast<intptr_t>(this)).obj());
  runner_ = runner;
}

SafetyNetCheck::~SafetyNetCheck() {
  Java_SafetyNetCheck_destroy(base::android::AttachCurrentThread(), java_obj_);
}

bool SafetyNetCheck::clientAttestation(const std::string& nonce, ClientAttestationCallback attest_callback) {
  attest_callback_ = std::move(attest_callback);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> jnonce = base::android::ConvertUTF8ToJavaString(env, nonce);
  return Java_SafetyNetCheck_clientAttestation(env, java_obj_, jnonce);
}

void SafetyNetCheck::clientAttestationResult(JNIEnv* env, const base::android::JavaRef<jobject>& jobj, jboolean jresult,
                                          const base::android::JavaParamRef<jstring>& jresult_string) {
  bool result = jresult;
  std::string result_string = base::android::ConvertJavaStringToUTF8(env, jresult_string);
  std::move(attest_callback_).Run(result, result_string);
  if (runner_ != nullptr) {
    runner_->jobFinished(this);
  }
}

SafetyNetCheckRunner::SafetyNetCheckRunner() {
}

SafetyNetCheckRunner::~SafetyNetCheckRunner() {
}

void SafetyNetCheckRunner::performSafetynetCheck(const std::string& nonce, ClientAttestationCallback attest_callback) {
  jobs_.push_back(std::make_unique<SafetyNetCheck>(this));
  if (!jobs_.back()->clientAttestation(nonce, std::move(attest_callback))) {
    jobFinished(jobs_.back().get());
  }
}

void SafetyNetCheckRunner::jobFinished(SafetyNetCheck* finished_job) {
  auto it = find_if(jobs_.begin(), jobs_.end(), [finished_job] (const std::unique_ptr<SafetyNetCheck>& job) { return (bool)(job.get() == finished_job); });
  if (it != jobs_.end()) {
    jobs_.erase(it);
  } else {
    NOTREACHED();
  }
}

}
