use super::*;
use crate::parser::state_machine::StateMachineConditions;

impl<S: TagHintSink> StateMachineConditions for TagScanner<S> {
    #[inline]
    #[must_use]
    fn is_appropriate_end_tag(&self) -> bool {
        self.tag_name_hash == self.last_start_tag_name_hash
    }

    #[inline]
    #[must_use]
    fn cdata_allowed(&self) -> bool {
        self.cdata_allowed
    }
}
