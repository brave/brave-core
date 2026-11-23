use crate::{
    descriptors::Desc,
    env::Env,
    errors::*,
    jni_str,
    objects::{JClass, JMethodID, JStaticMethodID},
    signature::MethodSignature,
    strings::JNIStr,
};

unsafe impl<'local, 'other_local, 'sig, 'sig_args, T, U, V> Desc<'local, JMethodID> for (T, U, V)
where
    T: Desc<'local, JClass<'other_local>>,
    U: AsRef<JNIStr>,
    V: AsRef<MethodSignature<'sig, 'sig_args>>,
{
    type Output = JMethodID;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        env.get_method_id(self.0, self.1, self.2)
    }
}

unsafe impl<'local, 'other_local, 'sig, 'sig_args, T, Signature> Desc<'local, JMethodID>
    for (T, Signature)
where
    T: Desc<'local, JClass<'other_local>>,
    Signature: AsRef<MethodSignature<'sig, 'sig_args>>,
{
    type Output = JMethodID;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        Desc::<JMethodID>::lookup((self.0, jni_str!("<init>"), self.1.as_ref()), env)
    }
}

unsafe impl<'local, 'other_local, 'sig, 'sig_args, T, U, V> Desc<'local, JStaticMethodID>
    for (T, U, V)
where
    T: Desc<'local, JClass<'other_local>>,
    U: AsRef<JNIStr>,
    V: AsRef<MethodSignature<'sig, 'sig_args>>,
{
    type Output = JStaticMethodID;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        env.get_static_method_id(self.0, self.1, self.2)
    }
}
