#![allow(clippy::needless_pass_by_value)]

mod ast;
mod attribute_matcher;
mod compiler;
mod error;
mod parser;
mod program;
mod stack;

use self::program::AddressRange;
use self::stack::StackDirective;
use crate::html::{LocalName, Namespace};
use crate::memory::{MemoryLimitExceededError, SharedMemoryLimiter};
use crate::transform_stream::AuxStartTagInfo;
use encoding_rs::Encoding;

pub use self::ast::*;
pub(crate) use self::attribute_matcher::AttributeMatcher;
pub(crate) use self::compiler::Compiler;
pub use self::error::SelectorError;
pub use self::parser::Selector;
pub(crate) use self::program::{ExecutionBranch, Program, TryExecResult};
pub(crate) use self::stack::{ChildCounter, ElementData, Stack, StackItem};

pub(crate) struct MatchInfo<P> {
    pub payload: P,
    pub with_content: bool,
}

pub(crate) type AuxStartTagInfoRequest<E, P> = Box<
    dyn FnOnce(
            &mut SelectorMatchingVm<E>,
            AuxStartTagInfo<'_>,
            &mut dyn FnMut(MatchInfo<P>),
        ) -> Result<(), MemoryLimitExceededError>
        + Send,
>;

pub(crate) enum VmError<E: ElementData, MatchPayload> {
    InfoRequest(AuxStartTagInfoRequest<E, MatchPayload>),
    MemoryLimitExceeded(MemoryLimitExceededError),
}

type RecoveryPointHandler<T, E, P> = fn(
    &mut SelectorMatchingVm<E>,
    &mut ExecutionCtx<'static, E>,
    &AttributeMatcher<'_>,
    T,
    &mut dyn FnMut(MatchInfo<P>),
);

#[derive(Default)]
struct JumpPtr {
    instr_set_idx: usize,
    offset: usize,
}

#[derive(Default)]
struct HereditaryJumpPtr {
    stack_offset: usize,
    instr_set_idx: usize,
    offset: usize,
}

struct Bailout<T> {
    at_addr: usize,
    recovery_point: T,
}

/// A container for tracking state from various places on the stack.
pub(crate) struct SelectorState<'i> {
    pub cumulative: &'i ChildCounter,
    pub typed: Option<&'i ChildCounter>,
}

struct ExecutionCtx<'i, E: ElementData> {
    stack_item: StackItem<'i, E>,
    with_content: bool,
    ns: Namespace,
    enable_esi_tags: bool,
}

impl<'i, E: ElementData> ExecutionCtx<'i, E> {
    #[inline]
    pub fn new(local_name: LocalName<'i>, ns: Namespace, enable_esi_tags: bool) -> Self {
        ExecutionCtx {
            stack_item: StackItem::new(local_name),
            with_content: true,
            ns,
            enable_esi_tags,
        }
    }

    pub fn add_execution_branch(
        &mut self,
        branch: &ExecutionBranch<E::MatchPayload>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        for &payload in &branch.matched_payload {
            let element_payload = self.stack_item.element_data.matched_payload_mut();

            if !element_payload.contains(&payload) {
                match_handler(MatchInfo {
                    payload,
                    with_content: self.with_content,
                });

                element_payload.insert(payload);
            }
        }

        if self.with_content {
            if let Some(ref jumps) = branch.jumps {
                self.stack_item.jumps.push(jumps.to_owned());
            }

            if let Some(ref hereditary_jumps) = branch.hereditary_jumps {
                self.stack_item
                    .hereditary_jumps
                    .push(hereditary_jumps.to_owned());
            }
        }
    }

    #[inline]
    pub fn into_owned(self) -> ExecutionCtx<'static, E> {
        ExecutionCtx {
            stack_item: self.stack_item.into_owned(),
            with_content: self.with_content,
            ns: self.ns,
            enable_esi_tags: self.enable_esi_tags,
        }
    }
}

macro_rules! aux_info_request {
    ($req:expr) => {
        Err(VmError::InfoRequest(Box::new($req)))
    };
}

pub(crate) struct SelectorMatchingVm<E: ElementData> {
    program: Program<E::MatchPayload>,
    stack: Stack<E>,
    enable_esi_tags: bool,
}

impl<E> SelectorMatchingVm<E>
where
    E: ElementData + Send,
{
    #[inline]
    #[must_use]
    pub fn new(
        ast: Ast<E::MatchPayload>,
        encoding: &'static Encoding,
        memory_limiter: SharedMemoryLimiter,
        enable_esi_tags: bool,
    ) -> Self {
        let program = Compiler::new(encoding).compile(ast);
        let enable_nth_of_type = program.enable_nth_of_type;

        Self {
            program,
            enable_esi_tags,
            stack: Stack::new(memory_limiter, enable_nth_of_type),
        }
    }

    pub fn exec_for_start_tag(
        &mut self,
        local_name: LocalName<'_>,
        ns: Namespace,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) -> Result<(), VmError<E, E::MatchPayload>> {
        use StackDirective::*;

        self.stack.add_child(&local_name);

        let mut ctx = ExecutionCtx::new(local_name, ns, self.enable_esi_tags);

        match Stack::get_stack_directive(&ctx.stack_item, ns, ctx.enable_esi_tags) {
            PopImmediately => {
                ctx.with_content = false;
                self.exec_without_attrs(ctx, match_handler)
            }
            PushIfNotSelfClosing => {
                let ctx = ctx.into_owned();

                aux_info_request!(move |this, aux_info, match_handler| this
                    .exec_after_immediate_aux_info_request(ctx, ns, aux_info, match_handler))
            }
            Push => self.exec_without_attrs(ctx, match_handler),
        }
    }

    #[inline]
    pub fn exec_for_end_tag(
        &mut self,
        local_name: LocalName<'_>,
        unmatched_element_data_handler: impl FnMut(E),
    ) {
        self.stack
            .pop_up_to(local_name, unmatched_element_data_handler);
    }

    #[inline]
    pub fn current_element_data_mut(&mut self) -> Option<&mut E> {
        self.stack.current_element_data_mut()
    }

    fn exec_after_immediate_aux_info_request(
        &mut self,
        mut ctx: ExecutionCtx<'static, E>,
        ns: Namespace,
        aux_info: AuxStartTagInfo<'_>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) -> Result<(), MemoryLimitExceededError> {
        let attr_matcher = AttributeMatcher::new(aux_info.input, aux_info.attr_buffer, ns);

        ctx.with_content = !aux_info.self_closing;

        self.exec_instr_set_with_attrs(
            &self.program.entry_points,
            &attr_matcher,
            &mut ctx,
            0,
            match_handler,
        );

        self.exec_jumps_with_attrs(&attr_matcher, &mut ctx, JumpPtr::default(), match_handler);

        self.exec_hereditary_jumps_with_attrs(
            &attr_matcher,
            &mut ctx,
            HereditaryJumpPtr::default(),
            match_handler,
        );

        if ctx.with_content {
            self.stack.push_item(ctx.stack_item)?;
        }

        Ok(())
    }

    fn bailout<T: 'static + Send>(
        ctx: ExecutionCtx<'_, E>,
        bailout: Bailout<T>,
        recovery_point_handler: RecoveryPointHandler<T, E, E::MatchPayload>,
    ) -> Result<(), VmError<E, E::MatchPayload>> {
        let mut ctx = ctx.into_owned();

        aux_info_request!(move |this, aux_info, match_handler| {
            let attr_matcher = AttributeMatcher::new(aux_info.input, aux_info.attr_buffer, ctx.ns);

            this.complete_instr_execution_with_attrs(
                bailout.at_addr,
                &attr_matcher,
                &mut ctx,
                match_handler,
            );

            recovery_point_handler(
                this,
                &mut ctx,
                &attr_matcher,
                bailout.recovery_point,
                match_handler,
            );

            if ctx.with_content {
                this.stack.push_item(ctx.stack_item)?;
            }

            Ok(())
        })
    }

    fn recover_after_bailout_in_entry_points(
        &mut self,
        ctx: &mut ExecutionCtx<'static, E>,
        attr_matcher: &AttributeMatcher<'_>,
        recovery_point: usize,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        self.exec_instr_set_with_attrs(
            &self.program.entry_points,
            attr_matcher,
            ctx,
            recovery_point,
            match_handler,
        );

        self.exec_jumps_with_attrs(attr_matcher, ctx, JumpPtr::default(), match_handler);

        self.exec_hereditary_jumps_with_attrs(
            attr_matcher,
            ctx,
            HereditaryJumpPtr::default(),
            match_handler,
        );
    }

    fn recover_after_bailout_in_jumps(
        &mut self,
        ctx: &mut ExecutionCtx<'static, E>,
        attr_matcher: &AttributeMatcher<'_>,
        recovery_point: JumpPtr,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        self.exec_jumps_with_attrs(attr_matcher, ctx, recovery_point, match_handler);

        self.exec_hereditary_jumps_with_attrs(
            attr_matcher,
            ctx,
            HereditaryJumpPtr::default(),
            match_handler,
        );
    }

    #[inline]
    fn recover_after_bailout_in_hereditary_jumps(
        &mut self,
        ctx: &mut ExecutionCtx<'static, E>,
        attr_matcher: &AttributeMatcher<'_>,
        recovery_point: HereditaryJumpPtr,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        self.exec_hereditary_jumps_with_attrs(attr_matcher, ctx, recovery_point, match_handler);
    }

    fn exec_without_attrs(
        &mut self,
        mut ctx: ExecutionCtx<'_, E>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) -> Result<(), VmError<E, E::MatchPayload>> {
        if let Err(b) = self.try_exec_instr_set_without_attrs(
            self.program.entry_points.clone(),
            &mut ctx,
            match_handler,
        ) {
            return Self::bailout(ctx, b, Self::recover_after_bailout_in_entry_points);
        }

        if let Err(b) = self.try_exec_jumps_without_attrs(&mut ctx, match_handler) {
            return Self::bailout(ctx, b, Self::recover_after_bailout_in_jumps);
        }

        if let Err(b) = self.try_exec_hereditary_jumps_without_attrs(&mut ctx, match_handler) {
            return Self::bailout(ctx, b, Self::recover_after_bailout_in_hereditary_jumps);
        }

        if ctx.with_content {
            self.stack
                .push_item(ctx.stack_item.into_owned())
                .map_err(VmError::MemoryLimitExceeded)
        } else {
            Ok(())
        }
    }

    #[inline]
    fn complete_instr_execution_with_attrs(
        &self,
        addr: usize,
        attr_matcher: &AttributeMatcher<'_>,
        ctx: &mut ExecutionCtx<'_, E>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        let state = self.stack.build_state(&ctx.stack_item.local_name);
        if let Some(branch) =
            self.program.instructions[addr].complete_exec_with_attrs(&state, attr_matcher)
        {
            ctx.add_execution_branch(branch, match_handler);
        }
    }

    #[inline]
    fn try_exec_instr_set_without_attrs(
        &self,
        addr_range: AddressRange,
        ctx: &mut ExecutionCtx<'_, E>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) -> Result<(), Bailout<usize>> {
        let start = addr_range.start;
        let state = self.stack.build_state(&ctx.stack_item.local_name);

        for addr in addr_range {
            match self.program.instructions[addr]
                .try_exec_without_attrs(&state, &ctx.stack_item.local_name)
            {
                TryExecResult::Branch(branch) => ctx.add_execution_branch(branch, match_handler),
                TryExecResult::AttributesRequired => {
                    return Err(Bailout {
                        at_addr: addr,
                        recovery_point: addr - start + 1,
                    });
                }
                TryExecResult::Fail => (),
            }
        }

        Ok(())
    }

    #[inline]
    fn exec_instr_set_with_attrs(
        &self,
        addr_range: &AddressRange,
        attr_matcher: &AttributeMatcher<'_>,
        ctx: &mut ExecutionCtx<'_, E>,
        offset: usize,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        let state = self.stack.build_state(&ctx.stack_item.local_name);
        for addr in addr_range.start + offset..addr_range.end {
            let instr = &self.program.instructions[addr];

            if let Some(branch) = instr.exec(&state, &ctx.stack_item.local_name, attr_matcher) {
                ctx.add_execution_branch(branch, match_handler);
            }
        }
    }

    fn try_exec_jumps_without_attrs(
        &self,
        ctx: &mut ExecutionCtx<'_, E>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) -> Result<(), Bailout<JumpPtr>> {
        if let Some(parent) = self.stack.items().last() {
            for (i, jumps) in parent.jumps.iter().enumerate() {
                self.try_exec_instr_set_without_attrs(jumps.clone(), ctx, match_handler)
                    .map_err(|b| Bailout {
                        at_addr: b.at_addr,
                        recovery_point: JumpPtr {
                            instr_set_idx: i,
                            offset: b.recovery_point,
                        },
                    })?;
            }
        }

        Ok(())
    }

    fn exec_jumps_with_attrs(
        &self,
        attr_matcher: &AttributeMatcher<'_>,
        ctx: &mut ExecutionCtx<'_, E>,
        ptr: JumpPtr,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        // NOTE: find pointed jumps instruction set and execute it with the offset.
        if let Some(parent) = self.stack.items().last() {
            if let Some(ptr_jumps) = parent.jumps.get(ptr.instr_set_idx) {
                self.exec_instr_set_with_attrs(
                    ptr_jumps,
                    attr_matcher,
                    ctx,
                    ptr.offset,
                    match_handler,
                );

                // NOTE: execute remaining jumps instruction sets as usual.
                for jumps in parent.jumps.iter().skip(ptr.instr_set_idx + 1) {
                    self.exec_instr_set_with_attrs(jumps, attr_matcher, ctx, 0, match_handler);
                }
            }
        }
    }

    fn try_exec_hereditary_jumps_without_attrs(
        &self,
        ctx: &mut ExecutionCtx<'_, E>,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) -> Result<(), Bailout<HereditaryJumpPtr>> {
        for (i, ancestor) in self.stack.items().iter().rev().enumerate() {
            for (j, jumps) in ancestor.hereditary_jumps.iter().cloned().enumerate() {
                self.try_exec_instr_set_without_attrs(jumps, ctx, match_handler)
                    .map_err(|b| Bailout {
                        at_addr: b.at_addr,
                        recovery_point: HereditaryJumpPtr {
                            stack_offset: i,
                            instr_set_idx: j,
                            offset: b.recovery_point,
                        },
                    })?;
            }

            if !ancestor.has_ancestor_with_hereditary_jumps {
                break;
            }
        }

        Ok(())
    }

    fn exec_hereditary_jumps_with_attrs(
        &self,
        attr_matcher: &AttributeMatcher<'_>,
        ctx: &mut ExecutionCtx<'_, E>,
        ptr: HereditaryJumpPtr,
        match_handler: &mut dyn FnMut(MatchInfo<E::MatchPayload>),
    ) {
        let items = self.stack.items();

        if items.is_empty() {
            return;
        }

        let ptr_ancestor_idx = items.len() - 1 - ptr.stack_offset;

        // NOTE: first find pointed ancestor, then jump instruction
        // set and execute it with the offset.
        if let Some(ptr_ancestor) = items.get(ptr_ancestor_idx) {
            if let Some(ptr_jumps) = ptr_ancestor.hereditary_jumps.get(ptr.instr_set_idx) {
                self.exec_instr_set_with_attrs(
                    ptr_jumps,
                    attr_matcher,
                    ctx,
                    ptr.offset,
                    match_handler,
                );

                // NOTE: execute the rest of jump instruction sets in the pointed ancestor as usual.
                for jumps in ptr_ancestor
                    .hereditary_jumps
                    .iter()
                    .skip(ptr.instr_set_idx + 1)
                {
                    self.exec_instr_set_with_attrs(jumps, attr_matcher, ctx, 0, match_handler);
                }
            }

            // NOTE: execute hereditary jumps in remaining ancestors as usual.
            if ptr_ancestor.has_ancestor_with_hereditary_jumps {
                for ancestor in items.iter().rev().skip(ptr.stack_offset + 1) {
                    for jumps in &ancestor.hereditary_jumps {
                        self.exec_instr_set_with_attrs(jumps, attr_matcher, ctx, 0, match_handler);
                    }

                    if !ancestor.has_ancestor_with_hereditary_jumps {
                        break;
                    }
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::base::SharedEncoding;
    use crate::errors::RewritingError;
    use crate::html::Namespace;
    use crate::memory::SharedMemoryLimiter;
    use crate::rewritable_units::{DocumentEnd, Token, TokenCaptureFlags};
    use crate::rewriter::AsciiCompatibleEncoding;
    use crate::transform_stream::{
        StartTagHandlingResult, TransformController, TransformStream, TransformStreamSettings,
    };
    use encoding_rs::{Encoding, UTF_8};
    use hashbrown::{HashMap, HashSet};

    struct Expectation {
        should_bailout: bool,
        should_match_with_content: bool,
        /// The indexes of selectors that match this tag
        matched_payload: HashSet<usize>,
    }

    #[derive(Default)]
    struct TestElementData(HashSet<usize>);

    impl ElementData for TestElementData {
        type MatchPayload = usize;

        fn matched_payload_mut(&mut self) -> &mut HashSet<usize> {
            &mut self.0
        }
    }

    struct TestTransformController<T: FnMut(&mut Token<'_>)>(T);

    impl<T: FnMut(&mut Token<'_>)> TransformController for TestTransformController<T> {
        fn initial_capture_flags(&self) -> TokenCaptureFlags {
            TokenCaptureFlags::all()
        }

        fn handle_start_tag(
            &mut self,
            _: LocalName<'_>,
            _: Namespace,
        ) -> StartTagHandlingResult<Self> {
            Ok(TokenCaptureFlags::NEXT_START_TAG)
        }

        fn handle_end_tag(&mut self, _: LocalName<'_>) -> TokenCaptureFlags {
            TokenCaptureFlags::all()
        }

        fn handle_end(&mut self, _: &mut DocumentEnd<'_>) -> Result<(), RewritingError> {
            Ok(())
        }

        fn handle_token(&mut self, token: &mut Token<'_>) -> Result<(), RewritingError> {
            (self.0)(token);
            Ok(())
        }

        fn should_emit_content(&self) -> bool {
            true
        }
    }

    pub fn test_with_token(
        html: &str,
        encoding: &'static Encoding,
        test_fn: impl FnMut(&mut Token<'_>),
    ) {
        let (html, _, has_unmappable_characters) = encoding.encode(html);

        // NOTE: there is no character in existence outside of ASCII range
        // that can be represented in all the ASCII-compatible encodings.
        // So, if test cases contains some non-ASCII characters that can't
        // be represented in the given encoding then we just skip it.
        // It is OK to do so, because our intention is not to test the
        // encoding library itself (it is already well tested), but test
        // how our own code works with non-ASCII characters.
        if has_unmappable_characters {
            return;
        }

        let mut transform_stream = TransformStream::new(TransformStreamSettings {
            transform_controller: TestTransformController(test_fn),
            output_sink: |_: &[u8]| {},
            preallocated_parsing_buffer_size: 0,
            encoding: SharedEncoding::new(AsciiCompatibleEncoding::new(encoding).unwrap()),
            memory_limiter: SharedMemoryLimiter::new(2048),
            strict: true,
        });

        transform_stream.write(&html).unwrap();
        transform_stream.end().unwrap();
    }

    macro_rules! set {
        ($($items:expr),*) => {
            vec![$($items),*].into_iter().collect::<HashSet<_>>()
        };
    }

    macro_rules! map {
        ($($items:expr),*) => {
            vec![$($items),*].into_iter().collect::<HashMap<_, _>>()
        };
    }

    macro_rules! local_name {
        ($t:expr) => {
            LocalName::from_str_without_replacements(&$t.name(), UTF_8).unwrap()
        };
    }

    // NOTE: these are macroses to preserve callsites on fails.
    macro_rules! create_vm {
        ($selectors:expr) => {{
            let mut ast = Ast::default();

            for (i, selector) in $selectors.iter().enumerate() {
                ast.add_selector(&selector.parse().unwrap(), i);
            }

            let memory_limiter = SharedMemoryLimiter::new(2048);
            let enable_esi_tags = false;
            let vm: SelectorMatchingVm<TestElementData> =
                SelectorMatchingVm::new(ast, UTF_8, memory_limiter, enable_esi_tags);

            vm
        }};
    }

    macro_rules! exec_for_start_tag_and_assert {
        ($vm:expr, $tag_html:expr, $ns:expr, $expectation:expr) => {
            test_with_token($tag_html, UTF_8, |t| {
                match t {
                    Token::StartTag(t) => {
                        let mut matched_payload = HashSet::default();

                        {
                            let mut match_handler = |m: MatchInfo<_>| {
                                assert_eq!(m.with_content, $expectation.should_match_with_content);
                                matched_payload.insert(m.payload);
                            };

                            let result =
                                $vm.exec_for_start_tag(local_name!(t), $ns, &mut match_handler);

                            if $expectation.should_bailout {
                                let aux_info_req = result.expect_err("Bailout expected");
                                let (input, attr_buffer) = t.raw_attributes();

                                match aux_info_req {
                                    VmError::InfoRequest(f) => f(
                                        &mut $vm,
                                        AuxStartTagInfo {
                                            input,
                                            attr_buffer,
                                            self_closing: t.self_closing(),
                                        },
                                        &mut match_handler,
                                    )
                                    .unwrap(),
                                    VmError::MemoryLimitExceeded(e) => panic!("{}", e),
                                }
                            } else {
                                // NOTE: can't use unwrap() or expect() here, because
                                // Debug is not implemented for the closure in the error type.
                                #[allow(clippy::match_wild_err_arm)]
                                match result {
                                    Ok(_) => (),
                                    Err(_) => panic!("Should match without bailout"),
                                }
                            }
                        }

                        assert_eq!(matched_payload, $expectation.matched_payload);
                    }
                    _ => panic!("Start tag expected"),
                }
            });
        };
    }

    macro_rules! exec_for_end_tag_and_assert {
        ($vm:expr, $tag_html:expr, $expected_unmatched_payload:expr) => {
            test_with_token($tag_html, UTF_8, |t| match t {
                Token::EndTag(t) => {
                    let mut unmatched_payload = HashMap::default();

                    $vm.exec_for_end_tag(local_name!(t), |elem_data: TestElementData| {
                        for payload in elem_data.0 {
                            unmatched_payload
                                .entry(payload)
                                .and_modify(|c| *c += 1)
                                .or_insert(1);
                        }
                    });

                    assert_eq!(unmatched_payload, $expected_unmatched_payload);
                }
                _ => panic!("End tag expected"),
            });
        };
    }

    #[test]
    fn html_elements() {
        let mut vm = create_vm!(&["a", "img.c1", ":not(a).c2"]);

        // Stack after:
        // - <a> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<a>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        // Void element.
        // Stack after:
        // - <a> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<img>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: false,
                matched_payload: set![],
            }
        );

        // Void element.
        // Stack after:
        // - <a> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<img class='c2 c1'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: false,
                matched_payload: set![1, 2],
            }
        );

        // Stack after:
        // - <a> (0)
        // - <a class='c2 c1'> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<a class='c1 c2'>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        // Stack after:
        // - <a> (0)
        // - <a class='c2 c1'> (0)
        // - <div class=c2> (2)
        exec_for_start_tag_and_assert!(
            vm,
            "<div class='c2'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![2],
            }
        );

        // Stack after:
        // - <a> (0)
        // - <a class='c2 c1'> (0)
        // - <div class=c2> (2)
        // - <h1 class='c1 c2 c3'> (2)
        exec_for_start_tag_and_assert!(
            vm,
            "<h1 class='c1 c2 c3'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![2],
            }
        );

        // Stack after:
        // Stack after:
        // - <a> (0)
        // - <a class='c2 c1'> (0)
        // - <div class=c2> (2)
        // - <h1 class='c1 c2 c3'> (2)
        exec_for_end_tag_and_assert!(vm, "</span>", map![]);

        // Stack after:
        // - <a> (0)
        exec_for_end_tag_and_assert!(vm, "</a>", map![(0, 1), (2, 2)]);

        // Stack after:
        // - <a> (0)
        exec_for_end_tag_and_assert!(vm, "</div>", map![]);

        // Stack after: empty
        exec_for_end_tag_and_assert!(vm, "</a>", map![(0, 1)]);
    }

    #[test]
    fn foreign_elements() {
        let mut vm = create_vm!(&["circle", "#foo"]);

        // Stack after:
        // - <svg>
        exec_for_start_tag_and_assert!(
            vm,
            "<svg>",
            Namespace::Svg,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Self-closing.
        // Stack after:
        // - <svg>
        exec_for_start_tag_and_assert!(
            vm,
            "<circle id=foo />",
            Namespace::Svg,
            Expectation {
                should_bailout: true,
                should_match_with_content: false,
                matched_payload: set![0, 1],
            }
        );

        // Self-closing.
        // Stack after:
        // - <svg>
        // - <circle> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<circle>",
            Namespace::Svg,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        // Stack after: empty
        exec_for_end_tag_and_assert!(vm, "</svg>", map![(0, 1)]);
    }

    #[test]
    fn entry_points() {
        let mut vm = create_vm!(&["div", "span[foo=bar]", "span#test"]);

        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        exec_for_start_tag_and_assert!(
            vm,
            "<span foo=bar>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![1],
            }
        );

        exec_for_start_tag_and_assert!(
            vm,
            "<span foo=bar id=test>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![1, 2],
            }
        );

        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 2), (2, 1)]);
    }

    #[test]
    fn entry_points_bailout_on_last_addr_in_set() {
        let mut vm = create_vm!(&["*", "span", "span[foo=bar]"]);

        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        exec_for_start_tag_and_assert!(
            vm,
            "<span foo=bar>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1, 2],
            }
        );
    }

    #[test]
    fn nth_child() {
        let mut vm = create_vm!(&["div:first-child", "div:nth-child(2n+1)"]);

        // Stack:
        // 0: html
        exec_for_start_tag_and_assert!(
            vm,
            "<html>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack:
        // 0: html
        // 1: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 1)]);

        // Stack:
        // 0: html
        // 1: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: false,
                matched_payload: set![],
            }
        );

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</div>", map![]);

        // Stack:
        // 0: html
        // 1: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![1],
            }
        );

        // Stack:
        // 0: html
        // 1: div
        // 2: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack:
        // 0: html
        // 1: div
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 1)]);

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</div>", map![(1, 1)]);

        // Stack:
        exec_for_end_tag_and_assert!(vm, "</html>", map![]);
    }

    #[test]
    fn nth_of_type() {
        let mut vm = create_vm!(&["div:first-of-type", "div:nth-of-type(2n+1)"]);

        // Stack:
        // 0: html
        exec_for_start_tag_and_assert!(
            vm,
            "<html>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack:
        // 0: html
        // 1: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 1)]);

        // Stack:
        // 0: html
        // 1: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: false,
                matched_payload: set![],
            }
        );

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</div>", map![]);

        // Stack:
        // 0: html
        // 1: a
        exec_for_start_tag_and_assert!(
            vm,
            "<a>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack:
        // 0: html
        // 1: a
        // 2: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack:
        // 0: html
        // 1: a
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 1)]);

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</a>", map![]);

        // Stack:
        // 0: html
        // 1: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![1],
            }
        );

        // Stack:
        // 0: html
        // 1: div
        // 2: div
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack:
        // 0: html
        // 1: div
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 1)]);

        // Stack:
        // 0: html
        exec_for_end_tag_and_assert!(vm, "</div>", map![(1, 1)]);

        // Stack:
        exec_for_end_tag_and_assert!(vm, "</html>", map![]);
    }

    #[test]
    fn jumps() {
        let mut vm = create_vm!(&["div > span", "div > #foo", ":not(span) > .c2 > .c3"]);

        // Stack after:
        // - <div>
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <div>
        // - <span> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        // Stack after:
        // - <div>
        // - <span> (0)
        // - <span>
        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <div>
        // - <span> (0)
        exec_for_end_tag_and_assert!(vm, "</span>", map![]);

        // Stack after:
        // - <div>
        exec_for_end_tag_and_assert!(vm, "</span>", map![(0, 1)]);

        // Stack after:
        // - <div>
        // - <div id=foo class=c2> (1)
        exec_for_start_tag_and_assert!(
            vm,
            "<div id=foo class=c2>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![1],
            }
        );

        // Stack after:
        // - <div>
        // - <div id=foo class=c2> (1)
        // - <span class=c3> (0, 2)
        exec_for_start_tag_and_assert!(
            vm,
            "<span class=c3>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 2],
            }
        );

        // Stack after is empty
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 1), (1, 1), (2, 1)]);
    }

    #[test]
    fn bailout_in_jumps_on_last_addr_in_set() {
        let mut vm = create_vm!(&["div > span", "div > *", "#foo > span", "#foo > ul.c1"]);

        // Stack after:
        // - <div id=foo>
        exec_for_start_tag_and_assert!(
            vm,
            "<div id=foo>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <div id=foo>
        // - <span> (0, 1, 2)
        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1, 2],
            }
        );

        // Stack after:
        // - <div id=foo>
        exec_for_end_tag_and_assert!(vm, "</span>", map![(0, 1), (1, 1), (2, 1)]);

        // Stack after:
        // - <div id=foo>
        // - <ul> (1)
        exec_for_start_tag_and_assert!(
            vm,
            "<ul>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![1],
            }
        );

        // Stack after:
        // - <div id=foo>
        exec_for_end_tag_and_assert!(vm, "</ul>", map![(1, 1)]);

        // Stack after:
        // - <div id=foo>
        // - <ul class=c1> (1, 3)
        exec_for_start_tag_and_assert!(
            vm,
            "<ul class=c1>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![1, 3],
            }
        );
    }

    #[test]
    fn hereditary_jumps() {
        let mut vm = create_vm!(&["div .c1", "#foo .c2 .c3"]);

        // Stack after:
        // - <div id=foo>
        exec_for_start_tag_and_assert!(
            vm,
            "<div id=foo>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <div id=foo>
        // - <div class='c1 c2'> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<div class='c1 c2'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        // Stack after:
        // - <div id=foo>
        // - <div class='c1 c2'> (0)
        // - <div class='c1 c2 c3'> (0, 1)
        exec_for_start_tag_and_assert!(
            vm,
            "<div class='c1 c2 c3'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack after:
        // - <div id=foo>
        // - <div class='c1 c2'> (0)
        // - <div class='c1 c2 c3'> (0, 1)
        // - <span class='c3'> (1)
        exec_for_start_tag_and_assert!(
            vm,
            "<span class='c3'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![1],
            }
        );

        // Stack after:
        // - <div id=foo>
        // - <div class='c1 c2'> (0)
        // - <div class='c1 c2 c3'> (0, 1)
        // - <span class='c3'> (1)
        // - <span class='c1 c3'> (0, 1)
        exec_for_start_tag_and_assert!(
            vm,
            "<span class='c1 c3'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1],
            }
        );

        // Stack after:
        // - <div id=foo>
        // - <div class='c1 c2'> (0)
        exec_for_end_tag_and_assert!(vm, "</div>", map![(0, 2), (1, 3)]);

        // Stack after:
        // - <div id=foo>
        // - <div class='c1'> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<span class='c1'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );
    }

    #[test]
    fn bailout_in_hereditary_jumps_in_first_ancestor() {
        let mut vm = create_vm!(&[
            "body div *",
            "body div span#foo",
            "body div span",
            "body * #foo"
        ]);

        // Stack after:
        // - <body>
        exec_for_start_tag_and_assert!(
            vm,
            "<body>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <div>
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <div>
        exec_for_start_tag_and_assert!(
            vm,
            "<img>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: false,
                matched_payload: set![0],
            }
        );

        // Stack after:
        // - <body>
        // - <div>
        // - <span id=foo> (0, 1, 2, 3)
        exec_for_start_tag_and_assert!(
            vm,
            "<span id=foo>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1, 2, 3],
            }
        );

        // Stack after:
        // - <body>
        // - <div>
        // - <span id=foo> (0, 1, 2, 3)
        // - <span> (0, 2)
        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 2],
            }
        );

        // Stack after is empty
        exec_for_end_tag_and_assert!(vm, "</body>", map![(0, 2), (1, 1), (2, 2), (3, 1)]);
    }

    #[test]
    fn bailout_in_hereditary_jumps_on_last_addr_in_last_set_of_last_ancestor() {
        let mut vm = create_vm!(&["body *", "body span#foo", "div *"]);

        // Stack after:
        // - <body>
        exec_for_start_tag_and_assert!(
            vm,
            "<body>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <div> (0)
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );

        // Stack after:
        // - <body>
        // - <div> (0)
        // - <div> (0, 1, 2)
        exec_for_start_tag_and_assert!(
            vm,
            "<span id=foo>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 1, 2],
            }
        );

        // Stack after:
        // - <body>
        // - <div> (0)
        // - <span> (0, 1, 2)
        // - <span> (0, 2)
        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0, 2],
            }
        );

        // Stack after is empty
        exec_for_end_tag_and_assert!(vm, "</body>", map![(0, 3), (1, 1), (2, 2)]);
    }

    #[test]
    fn compound_selector() {
        let mut vm = create_vm!(&["body > span#foo .c1 .c2"]);

        // Stack after:
        // - <body>
        exec_for_start_tag_and_assert!(
            vm,
            "<body>",
            Namespace::Html,
            Expectation {
                should_bailout: false,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <span>
        exec_for_start_tag_and_assert!(
            vm,
            "<span>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        exec_for_end_tag_and_assert!(vm, "</span>", map![]);

        // Stack after:
        // - <body>
        // - <span id=foo>
        exec_for_start_tag_and_assert!(
            vm,
            "<span id=foo>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <span id=foo>
        // - <div>
        exec_for_start_tag_and_assert!(
            vm,
            "<div>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <span id=foo>
        // - <div>
        // - <ul class='bar c1'>
        exec_for_start_tag_and_assert!(
            vm,
            "<ul class='bar c1'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <span id=foo>
        // - <div>
        // - <ul class='bar c1'>
        // - <li class='c3'>
        exec_for_start_tag_and_assert!(
            vm,
            "<li class='c3'>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![],
            }
        );

        // Stack after:
        // - <body>
        // - <span id=foo>
        // - <div>
        // - <ul class='bar c1'>
        // - <li class='c3'>
        // - <span class=c2>
        exec_for_start_tag_and_assert!(
            vm,
            "<span class=c2>",
            Namespace::Html,
            Expectation {
                should_bailout: true,
                should_match_with_content: true,
                matched_payload: set![0],
            }
        );
    }
}
