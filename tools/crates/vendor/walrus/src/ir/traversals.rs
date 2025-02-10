//! Implementations of various IR traversals.

use crate::ir::*;

/// Perform an intra-procedural, depth-first, in-order traversal of the IR.
///
/// * *Intra-procedural*: Only traverses IR within a function. Does not cross
///   function boundaries (although it will report edges to other functions via
///   `visit_function_id` calls on the visitor, so you can use this as a
///   building block for making global, inter-procedural analyses).
///
/// * *Depth-first, in-order*: Visits instructions and instruction sequences in
///   the order they are defined and nested. See [Wikipedia][in-order] for
///   details.
///
/// Calls `visitor` methods for every instruction, instruction sequence, and
/// resource that the traversal visits.
///
/// The traversals begins at the `start` instruction sequence and goes from
/// there. To traverse everything in a function, pass `func.entry_block()` as
/// `start`.
///
/// This implementation is iterative &mdash; not recursive &mdash; and so it
/// will not blow the call stack on deeply nested Wasm (although it may still
/// OOM).
///
/// [in-order]: https://en.wikipedia.org/wiki/Tree_traversal#In-order_(LNR)
///
/// # Example
///
/// This example counts the number of instruction sequences in a function.
///
/// ```no_run
/// use walrus::LocalFunction;
/// use walrus::ir::*;
///
/// #[derive(Default)]
/// struct CountInstructionSequences {
///     count: usize,
/// }
///
/// impl<'instr> Visitor<'instr> for CountInstructionSequences {
///     fn start_instr_seq(&mut self, _: &'instr InstrSeq) {
///         self.count += 1;
///     }
/// }
///
/// // Get a function from somewhere.
/// # let get_my_function = || unimplemented!();
/// let my_func: &LocalFunction = get_my_function();
///
/// // Create our visitor.
/// let mut visitor = CountInstructionSequences::default();
///
/// // Traverse everything in the function with our visitor.
/// dfs_in_order(&mut visitor, my_func, my_func.entry_block());
///
/// // Use the aggregate results that `visitor` built up.
/// println!("The number of instruction sequences in `my_func` is {}", visitor.count);
/// ```
pub fn dfs_in_order<'instr>(
    visitor: &mut impl Visitor<'instr>,
    func: &'instr LocalFunction,
    start: InstrSeqId,
) {
    // The stack of instruction sequences we still need to visit, and how far
    // along in the instruction sequence we are.
    let mut stack: Vec<(InstrSeqId, usize)> = vec![(start, 0)];

    'traversing_blocks: while let Some((seq_id, index)) = stack.pop() {
        let seq = func.block(seq_id);

        if index == 0 {
            // If the `index` is zero, then we haven't processed any
            // instructions in this sequence yet, and it is the first time we
            // are entering it, so let the visitor know.
            visitor.start_instr_seq(seq);
            seq.visit(visitor);
        }

        'traversing_instrs: for (index, (instr, loc)) in seq.instrs.iter().enumerate().skip(index) {
            // Visit this instruction.
            log::trace!("dfs_in_order: visit_instr({:?})", instr);
            visitor.visit_instr(instr, loc);

            // Visit every other resource that this instruction references,
            // e.g. `MemoryId`s, `FunctionId`s and all that.
            log::trace!("dfs_in_order: ({:?}).visit(..)", instr);
            instr.visit(visitor);

            match instr {
                // Pause iteration through this sequence's instructions and
                // enqueue `seq` to be traversed next before continuing with
                // this one where we left off.
                Instr::Block(Block { seq }) | Instr::Loop(Loop { seq }) => {
                    stack.push((seq_id, index + 1));
                    stack.push((*seq, 0));
                    continue 'traversing_blocks;
                }

                // Pause iteration through this sequence's instructions.
                // Traverse the consequent and then the alternative.
                Instr::IfElse(IfElse {
                    consequent,
                    alternative,
                }) => {
                    stack.push((seq_id, index + 1));
                    stack.push((*alternative, 0));
                    stack.push((*consequent, 0));
                    continue 'traversing_blocks;
                }

                // No other instructions define new instruction sequences, so
                // continue to the next instruction.
                _ => continue 'traversing_instrs,
            }
        }

        // If we made it through the whole loop above, then we processed every
        // instruction in the sequence, and its nested sequences, so we are
        // finished with it!
        visitor.end_instr_seq(seq);
    }
}

/// Perform an intra-procedural, depth-first, pre-order, mutable traversal of
/// the IR.
///
/// * *Intra-procedural*: Only traverses IR within a function. Does not cross
///   function boundaries (although it will report edges to other functions via
///   `visit_function_id` calls on the visitor, so you can use this as a
///   building block for making global, inter-procedural analyses).
///
/// * *Depth-first, pre-order*: Visits instructions and instruction sequences in
///   a top-down manner, where all instructions in a parent sequences are
///   visited before child sequences. See [Wikipedia][pre-order] for details.
///
/// Calls `visitor` methods for every instruction, instruction sequence, and
/// resource that the traversal visits.
///
/// The traversals begins at the `start` instruction sequence and goes from
/// there. To traverse everything in a function, pass `func.entry_block()` as
/// `start`.
///
/// This implementation is iterative &mdash; not recursive &mdash; and so it
/// will not blow the call stack on deeply nested Wasm (although it may still
/// OOM).
///
/// [pre-order]: https://en.wikipedia.org/wiki/Tree_traversal#Pre-order_(NLR)
///
/// # Example
///
/// This example walks the IR and adds one to all `i32.const`'s values.
///
/// ```no_run
/// use walrus::LocalFunction;
/// use walrus::ir::*;
///
/// #[derive(Default)]
/// struct AddOneToI32Consts;
///
/// impl VisitorMut for AddOneToI32Consts {
///     fn visit_const_mut(&mut self, c: &mut Const) {
///         match &mut c.value {
///             Value::I32(x) => {
///                 *x += 1;
///             }
///             _ => {},
///         }
///     }
/// }
///
/// // Get a function from somewhere.
/// # let get_my_function = || unimplemented!();
/// let my_func: &mut LocalFunction = get_my_function();
///
/// // Create our visitor.
/// let mut visitor = AddOneToI32Consts::default();
///
/// // Traverse and mutate everything in the function with our visitor.
/// dfs_pre_order_mut(&mut visitor, my_func, my_func.entry_block());
/// ```
pub fn dfs_pre_order_mut(
    visitor: &mut impl VisitorMut,
    func: &mut LocalFunction,
    start: InstrSeqId,
) {
    let mut stack = vec![start];

    while let Some(seq_id) = stack.pop() {
        let seq = func.block_mut(seq_id);
        visitor.start_instr_seq_mut(seq);
        seq.visit_mut(visitor);

        for (instr, loc) in &mut seq.instrs {
            visitor.visit_instr_mut(instr, loc);
            instr.visit_mut(visitor);

            match instr {
                Instr::Block(Block { seq }) | Instr::Loop(Loop { seq }) => {
                    stack.push(*seq);
                }

                Instr::IfElse(IfElse {
                    consequent,
                    alternative,
                }) => {
                    stack.push(*alternative);
                    stack.push(*consequent);
                }

                _ => {}
            }
        }

        visitor.end_instr_seq_mut(seq);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Default)]
    struct TestVisitor {
        visits: Vec<String>,
    }

    impl TestVisitor {
        fn push(&mut self, s: impl ToString) {
            self.visits.push(s.to_string());
        }
    }

    impl<'a> Visitor<'a> for TestVisitor {
        fn start_instr_seq(&mut self, _: &'a InstrSeq) {
            self.push("start");
        }

        fn end_instr_seq(&mut self, _: &'a InstrSeq) {
            self.push("end");
        }

        fn visit_const(&mut self, c: &Const) {
            match c.value {
                Value::I32(x) => self.push(x),
                _ => unreachable!(),
            }
        }

        fn visit_drop(&mut self, _: &Drop) {
            self.push("drop");
        }

        fn visit_block(&mut self, _: &Block) {
            self.push("block");
        }

        fn visit_if_else(&mut self, _: &IfElse) {
            self.push("if-else");
        }
    }

    impl VisitorMut for TestVisitor {
        fn start_instr_seq_mut(&mut self, _: &mut InstrSeq) {
            self.push("start");
        }

        fn end_instr_seq_mut(&mut self, _: &mut InstrSeq) {
            self.push("end");
        }

        fn visit_const_mut(&mut self, c: &mut Const) {
            match &mut c.value {
                Value::I32(x) => {
                    self.push(*x);
                    *x += 1;
                }
                _ => unreachable!(),
            }
        }

        fn visit_drop_mut(&mut self, _: &mut Drop) {
            self.push("drop");
        }

        fn visit_block_mut(&mut self, _: &mut Block) {
            self.push("block");
        }

        fn visit_if_else_mut(&mut self, _: &mut IfElse) {
            self.push("if-else");
        }
    }

    fn make_test_func(module: &mut crate::Module) -> &mut LocalFunction {
        let block_ty = module.types.add(&[], &[]);
        let mut builder = crate::FunctionBuilder::new(&mut module.types, &[], &[]);

        builder
            .func_body()
            .i32_const(1)
            .drop()
            .block(block_ty, |block| {
                block
                    .i32_const(2)
                    .drop()
                    .if_else(
                        block_ty,
                        |then| {
                            then.i32_const(3).drop();
                        },
                        |else_| {
                            else_.i32_const(4).drop();
                        },
                    )
                    .i32_const(5)
                    .drop();
            })
            .i32_const(6)
            .drop();

        let func_id = builder.finish(vec![], &mut module.funcs);
        module.funcs.get_mut(func_id).kind.unwrap_local_mut()
    }

    #[test]
    fn dfs_in_order() {
        let mut module = crate::Module::default();
        let func = make_test_func(&mut module);

        let mut visitor = TestVisitor::default();
        crate::ir::dfs_in_order(&mut visitor, func, func.entry_block());

        let expected = [
            "start", "1", "drop", "block", "start", "2", "drop", "if-else", "start", "3", "drop",
            "end", "start", "4", "drop", "end", "5", "drop", "end", "6", "drop", "end",
        ];

        assert_eq!(
            visitor.visits,
            expected.iter().map(|s| s.to_string()).collect::<Vec<_>>()
        );
    }

    #[test]
    fn dfs_pre_order_mut() {
        let mut module = crate::Module::default();
        let func = make_test_func(&mut module);

        let mut visitor = TestVisitor::default();
        crate::ir::dfs_pre_order_mut(&mut visitor, func, func.entry_block());

        let mut expected = vec![];
        // function entry
        expected.extend(vec!["start", "1", "drop", "block", "6", "drop", "end"]);
        // block
        expected.extend(vec!["start", "2", "drop", "if-else", "5", "drop", "end"]);
        // consequent
        expected.extend(vec!["start", "3", "drop", "end"]);
        // alternative
        expected.extend(vec!["start", "4", "drop", "end"]);

        assert_eq!(
            visitor.visits,
            expected.iter().map(|s| s.to_string()).collect::<Vec<_>>()
        );

        // And then check that the increments of the constant values did indeed
        // take effect.

        visitor.visits.clear();
        crate::ir::dfs_in_order(&mut visitor, func, func.entry_block());

        let expected = [
            "start", "2", "drop", "block", "start", "3", "drop", "if-else", "start", "4", "drop",
            "end", "start", "5", "drop", "end", "6", "drop", "end", "7", "drop", "end",
        ];

        assert_eq!(
            visitor.visits,
            expected.iter().map(|s| s.to_string()).collect::<Vec<_>>()
        );
    }
}
