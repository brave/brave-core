use std::io::Error;

use html5ever::serialize::{Serialize, Serializer, TraversalScope};

use crate::ElementRef;

impl Serialize for ElementRef<'_> {
    fn serialize<S: Serializer>(
        &self,
        serializer: &mut S,
        traversal_scope: TraversalScope,
    ) -> Result<(), Error> {
        crate::node::serializable::serialize(**self, serializer, traversal_scope)
    }
}
