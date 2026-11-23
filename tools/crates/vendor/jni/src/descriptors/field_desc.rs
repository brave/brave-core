use crate::{
    descriptors::Desc,
    env::Env,
    errors::*,
    objects::{JClass, JFieldID, JStaticFieldID},
    signature::FieldSignature,
    strings::JNIStr,
};

unsafe impl<'local, 'other_local, 'sig, T, U, V> Desc<'local, JFieldID> for (T, U, V)
where
    T: Desc<'local, JClass<'other_local>>,
    U: AsRef<JNIStr>,
    V: AsRef<FieldSignature<'sig>>,
{
    type Output = JFieldID;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        env.get_field_id(self.0, self.1, self.2)
    }
}

unsafe impl<'local, 'other_local, 'sig, T, U, V> Desc<'local, JStaticFieldID> for (T, U, V)
where
    T: Desc<'local, JClass<'other_local>>,
    U: AsRef<JNIStr>,
    V: AsRef<FieldSignature<'sig>>,
{
    type Output = JStaticFieldID;

    fn lookup(self, env: &mut Env<'local>) -> Result<Self::Output> {
        env.get_static_field_id(self.0, self.1, self.2)
    }
}
