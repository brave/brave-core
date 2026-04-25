use super::attribute_matcher::AttributeMatcher;
use super::compiler::{CompiledAttributeExpr, CompiledLocalNameExpr};
use super::SelectorState;
use crate::html::LocalName;
use hashbrown::HashSet;
use std::hash::Hash;
use std::ops::Range;

pub(crate) type AddressRange = Range<usize>;

#[derive(Debug, PartialEq, Eq)]
pub(crate) struct ExecutionBranch<P>
where
    P: Hash + Eq,
{
    pub matched_payload: HashSet<P>,
    pub jumps: Option<AddressRange>,
    pub hereditary_jumps: Option<AddressRange>,
}

/// The result of trying to execute an instruction without having parsed all attributes
pub(crate) enum TryExecResult<'i, P>
where
    P: Hash + Eq,
{
    /// A successful match, contains the branch to move to
    Branch(&'i ExecutionBranch<P>),
    /// A partially successful match, but requires attributes to complete
    AttributesRequired,
    /// A failed match, doesn't require attributes to complete
    Fail,
}

pub(crate) struct Instruction<P>
where
    P: Hash + Eq,
{
    pub associated_branch: ExecutionBranch<P>,
    pub local_name_exprs: Box<[CompiledLocalNameExpr]>,
    pub attribute_exprs: Box<[CompiledAttributeExpr]>,
}

impl<P> Instruction<P>
where
    P: Hash + Eq,
{
    pub fn try_exec_without_attrs<'i>(
        &'i self,
        state: &SelectorState<'_>,
        local_name: &LocalName<'_>,
    ) -> TryExecResult<'i, P> {
        if self.local_name_exprs.iter().all(|e| e(state, local_name)) {
            if self.attribute_exprs.is_empty() {
                TryExecResult::Branch(&self.associated_branch)
            } else {
                TryExecResult::AttributesRequired
            }
        } else {
            TryExecResult::Fail
        }
    }

    pub fn complete_exec_with_attrs<'i>(
        &'i self,
        state: &SelectorState<'_>,
        attr_matcher: &AttributeMatcher<'_>,
    ) -> Option<&'i ExecutionBranch<P>> {
        if self.attribute_exprs.iter().all(|e| e(state, attr_matcher)) {
            Some(&self.associated_branch)
        } else {
            None
        }
    }

    pub fn exec<'i>(
        &'i self,
        state: &SelectorState<'_>,
        local_name: &LocalName<'_>,
        attr_matcher: &AttributeMatcher<'_>,
    ) -> Option<&'i ExecutionBranch<P>> {
        let is_match = self.local_name_exprs.iter().all(|e| e(state, local_name))
            && self.attribute_exprs.iter().all(|e| e(state, attr_matcher));

        if is_match {
            Some(&self.associated_branch)
        } else {
            None
        }
    }
}

pub(crate) struct Program<P>
where
    P: Hash + Eq,
{
    pub instructions: Box<[Instruction<P>]>,
    pub entry_points: AddressRange,
    /// Enables tracking child types for nth-of-type selectors.
    /// This is disabled if no nth-of-type selectors are used in the program.
    pub enable_nth_of_type: bool,
}
