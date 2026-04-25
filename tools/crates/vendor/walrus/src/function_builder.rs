use crate::ir::*;
use crate::tombstone_arena::TombstoneArena;
use crate::{FunctionId, LocalFunction, ModuleFunctions, ModuleTypes, TypeId, ValType};
use std::ops::{Deref, DerefMut};

/// Build instances of `LocalFunction`.
///
/// # Example
///
/// * For micro-examples, see the docs for various `FunctionBuilder` and
///   `InstrSeqBuilder` methods.
///
/// * For a bit more realistic example, see
///   [`examples/build-wasm-from-scratch.rs`](https://github.com/rustwasm/walrus/blob/master/examples/build-wasm-from-scratch.rs).
#[derive(Debug)]
pub struct FunctionBuilder {
    pub(crate) arena: TombstoneArena<InstrSeq>,
    pub(crate) ty: TypeId,
    pub(crate) entry: Option<InstrSeqId>,
    pub(crate) name: Option<String>,
}

impl FunctionBuilder {
    /// Creates a new, empty function builder.
    pub fn new(
        types: &mut ModuleTypes,
        params: &[ValType],
        results: &[ValType],
    ) -> FunctionBuilder {
        let ty = types.add(params, results);
        let mut builder = FunctionBuilder::without_entry(ty);
        let entry_ty = types.add_entry_ty(results);
        let entry = builder.dangling_instr_seq(entry_ty).id;
        builder.entry = Some(entry);
        builder
    }

    /// Create a builder that doesn't have a function body / entry
    /// sequence. Callers are responsible for initializing its entry.
    pub(crate) fn without_entry(ty: TypeId) -> FunctionBuilder {
        let arena = TombstoneArena::<InstrSeq>::default();
        FunctionBuilder {
            arena,
            ty,
            entry: None,
            name: None,
        }
    }

    /// Set function name.
    pub fn name(&mut self, function_name: String) -> &mut FunctionBuilder {
        self.name = Some(function_name);
        self
    }

    /// Get the id of this function's body's instruction sequence.
    pub fn func_body_id(&self) -> InstrSeqId {
        self.entry.unwrap()
    }

    /// Get a `InstrSeqBuilder` for building and mutating this function's body.
    pub fn func_body(&mut self) -> InstrSeqBuilder {
        let entry = self.entry.unwrap();
        self.instr_seq(entry)
    }

    /// Continue building and mutating an existing instruction sequence.
    ///
    /// # Example
    ///
    /// ```
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// let mut block = builder.dangling_instr_seq(None);
    /// let id = block.id();
    /// // Build up the block some.
    /// block
    ///     .f64_const(1337.0)
    ///     .drop();
    ///
    /// // Do some other stuff...
    /// drop(block);
    ///
    /// // Use `instr_seq` to get the builder for the block again, and build
    /// // some more things onto it.
    /// let mut block = builder.instr_seq(id);
    /// block
    ///     .i32_const(42)
    ///     .drop();
    /// ```
    pub fn instr_seq(&mut self, id: InstrSeqId) -> InstrSeqBuilder {
        InstrSeqBuilder { id, builder: self }
    }

    /// Create a new instruction sequence that is unreachable.
    ///
    /// It is your responsibility to
    ///
    /// * make a `Instr::Block`, `Instr::Loop`, or `Instr::IfElse` that uses
    ///   this instruction sequence, and
    ///
    /// * append that `Instr` into a parent instruction sequence via
    ///   `InstrSeqBuilder::instr` or `InstrSeqBuilder::instr_at`
    ///
    /// or else this built up instruction sequence will never be used.
    ///
    /// # Example
    ///
    /// ```
    /// use walrus::ir::*;
    ///
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// // Create an empty, dangling instruction sequemce.
    /// let mut seq = builder.dangling_instr_seq(None);
    /// let seq_id = seq.id();
    ///
    /// // Do stuff with the sequence...
    /// drop(seq);
    ///
    /// // Finally, make our instruction sequence reachable by adding an
    /// // block/loop/if-else instruction that uses it to a reachable instruction
    /// // sequence.
    /// builder
    ///     .func_body()
    ///     .instr(Block { seq: seq_id });
    /// ```
    pub fn dangling_instr_seq(&mut self, ty: impl Into<InstrSeqType>) -> InstrSeqBuilder {
        let ty = ty.into();
        let id = self.arena.alloc_with_id(|id| InstrSeq::new(id, ty));
        InstrSeqBuilder { id, builder: self }
    }

    /// Finishes this builder, wrapping it all up and inserting it into the
    /// specified `Module`.
    ///
    /// # Example
    ///
    /// ```
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// builder
    ///     .func_body()
    ///     .i32_const(1234)
    ///     .drop();
    ///
    /// let function_id = builder.finish(vec![], &mut module.funcs);
    /// # let _ = function_id;
    /// ```
    pub fn finish(self, args: Vec<LocalId>, funcs: &mut ModuleFunctions) -> FunctionId {
        let func = LocalFunction::new(args, self);
        funcs.add_local(func)
    }

    /// Returns the [crate::LocalFunction] built by this builder.
    pub fn local_func(self, args: Vec<LocalId>) -> LocalFunction {
        LocalFunction::new(args, self)
    }
}

/// A builder returned by instruction sequence-construction methods to build up
/// instructions within a block/loop/if-else over time.
#[derive(Debug)]
pub struct InstrSeqBuilder<'a> {
    id: InstrSeqId,
    builder: &'a mut FunctionBuilder,
}

impl InstrSeqBuilder<'_> {
    /// Returns the id of the instruction sequence that we're building.
    #[inline]
    pub fn id(&self) -> InstrSeqId {
        self.id
    }

    /// Get this instruction sequence's instructions.
    pub fn instrs(&self) -> &[(Instr, InstrLocId)] {
        &self.builder.arena[self.id]
    }

    /// Get this instruction sequence's instructions mutably.
    pub fn instrs_mut(&mut self) -> &mut Vec<(Instr, InstrLocId)> {
        &mut self.builder.arena[self.id].instrs
    }

    /// Pushes a new instruction onto this builder's sequence.
    #[inline]
    pub fn instr(&mut self, instr: impl Into<Instr>) -> &mut Self {
        self.builder.arena[self.id]
            .instrs
            .push((instr.into(), Default::default()));
        self
    }

    /// Splice a new instruction into this builder's sequence at the given index.
    ///
    /// # Panics
    ///
    /// Panics if `position > self.instrs.len()`.
    #[inline]
    pub fn instr_at(&mut self, position: usize, instr: impl Into<Instr>) -> &mut Self {
        self.builder.arena[self.id]
            .instrs
            .insert(position, (instr.into(), Default::default()));
        self
    }

    /// Creates an `i32.const` instruction for the specified value.
    #[inline]
    pub fn i32_const(&mut self, val: i32) -> &mut Self {
        self.const_(Value::I32(val))
    }

    /// Creates an `i64.const` instruction for the specified value.
    #[inline]
    pub fn i64_const(&mut self, val: i64) -> &mut Self {
        self.const_(Value::I64(val))
    }

    /// Creates an `f32.const` instruction for the specified value
    #[inline]
    pub fn f32_const(&mut self, val: f32) -> &mut Self {
        self.const_(Value::F32(val))
    }

    /// Creates an `f64.const` instruction for the specified value
    #[inline]
    pub fn f64_const(&mut self, val: f64) -> &mut Self {
        self.const_(Value::F64(val))
    }

    /// Append a new, nested `block ... end` to this builder's sequence.
    ///
    /// # Example:
    ///
    /// ```
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// // Append the following WAT to the function:
    /// //
    /// //     block
    /// //       i32.const 1337
    /// //       drop
    /// //     end
    /// builder
    ///     .func_body()
    ///     .block(None, |block| {
    ///         block
    ///             .i32_const(1337)
    ///             .drop();
    ///     });
    /// ```
    pub fn block(
        &mut self,
        ty: impl Into<InstrSeqType>,
        make_block: impl FnOnce(&mut InstrSeqBuilder),
    ) -> &mut Self {
        let mut builder = self.dangling_instr_seq(ty);
        make_block(&mut builder);
        let seq = builder.id;
        self.instr(Block { seq })
    }

    /// Append a new, nested `block ... end` to this builder's sequence.
    ///
    /// # Example:
    ///
    /// ```
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// // Make the function's body be a single `unreachable` instruction.
    /// builder
    ///     .func_body()
    ///     .unreachable();
    ///
    /// // Splice the following WAT into the function, before the `unreachable`:
    /// //
    /// //     block
    /// //       i32.const 1337
    /// //       drop
    /// //     end
    /// builder
    ///     .func_body()
    ///     .block_at(0, None, |block| {
    ///         block
    ///             .i32_const(1337)
    ///             .drop();
    ///     });
    /// ```
    pub fn block_at(
        &mut self,
        position: usize,
        ty: impl Into<InstrSeqType>,
        make_block: impl FnOnce(&mut InstrSeqBuilder),
    ) -> &mut Self {
        let mut builder = self.dangling_instr_seq(ty);
        make_block(&mut builder);
        let seq = builder.id;
        self.instr_at(position, Block { seq })
    }

    /// Create a new `loop ... end` instruction sequence.
    ///
    /// # Example
    ///
    /// ```
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// // Append the following WAT to the function:
    /// //
    /// //     block
    /// //       i32.const 1337
    /// //       drop
    /// //     end
    /// builder
    ///     .func_body()
    ///     .loop_(None, |loop_| {
    ///         loop_
    ///             .i32_const(1337)
    ///             .drop();
    ///     });
    /// ```
    pub fn loop_(
        &mut self,
        ty: impl Into<InstrSeqType>,
        make_loop: impl FnOnce(&mut InstrSeqBuilder),
    ) -> &mut Self {
        let mut builder = self.dangling_instr_seq(ty);
        make_loop(&mut builder);
        let seq = builder.id;
        self.instr(Loop { seq })
    }

    /// Splice a new `loop ... end` into this instruction sequence at the given
    /// position.
    ///
    /// # Example
    ///
    /// ```
    /// let mut module = walrus::Module::default();
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// // Make the function's body be a single `unreachable` instruction.
    /// builder
    ///     .func_body()
    ///     .unreachable();
    ///
    /// // Splice the following WAT into the function, before the `unreachable`:
    /// //
    /// //     loop
    /// //       i32.const 1337
    /// //       drop
    /// //     end
    /// builder
    ///     .func_body()
    ///     .loop_at(0, None, |loop_| {
    ///         loop_
    ///             .i32_const(1337)
    ///             .drop();
    ///     });
    /// ```
    pub fn loop_at(
        &mut self,
        position: usize,
        ty: impl Into<InstrSeqType>,
        make_loop: impl FnOnce(&mut InstrSeqBuilder),
    ) -> &mut Self {
        let mut builder = self.dangling_instr_seq(ty);
        make_loop(&mut builder);
        let seq = builder.id;
        self.instr_at(position, Loop { seq })
    }

    /// Build a new `if <consequent> else <alternative> end` instruction
    /// sequence.
    ///
    /// # Example
    ///
    /// ```
    /// use walrus::ValType;
    ///
    /// let mut module = walrus::Module::default();
    ///
    /// let ty = module.types.add(&[], &[ValType::I32]);
    /// let (flip_coin, _) = module.add_import_func("flip", "coin", ty);
    ///
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    /// builder
    ///     .func_body()
    ///     // (if (call $flip_coin)
    ///     //   (then (i32.const 12))
    ///     //   (else (i32.const 34)))
    ///     .call(flip_coin)
    ///     .if_else(
    ///         ValType::I32,
    ///         |then| {
    ///             then.i32_const(12);
    ///         },
    ///         |else_| {
    ///             else_.i32_const(34);
    ///         },
    ///     );
    /// ```
    pub fn if_else(
        &mut self,
        ty: impl Into<InstrSeqType>,
        consequent: impl FnOnce(&mut InstrSeqBuilder),
        alternative: impl FnOnce(&mut InstrSeqBuilder),
    ) -> &mut Self {
        let ty = ty.into();

        let consequent = {
            let mut builder = self.dangling_instr_seq(ty);
            consequent(&mut builder);
            builder.id
        };

        let alternative = {
            let mut builder = self.dangling_instr_seq(ty);
            alternative(&mut builder);
            builder.id
        };

        self.instr(IfElse {
            consequent,
            alternative,
        })
    }

    /// Splice a new `if <consequent> else <alternative> end` into this
    /// instruction sequence at the given position.
    ///
    /// # Example
    ///
    /// ```
    /// use walrus::ValType;
    ///
    /// let mut module = walrus::Module::default();
    ///
    /// let ty = module.types.add(&[], &[ValType::I32]);
    /// let (flip_coin, _) = module.add_import_func("flip", "coin", ty);
    ///
    /// let mut builder = walrus::FunctionBuilder::new(&mut module.types, &[], &[]);
    ///
    /// builder
    ///     .func_body()
    ///     .call(flip_coin)
    ///     .unreachable();
    ///
    /// // Splice an if/else after the `call` and before the `unreachable`.
    /// builder
    ///     .func_body()
    ///     .if_else_at(
    ///         1,
    ///         ValType::I32,
    ///         |then| {
    ///             then.i32_const(12);
    ///         },
    ///         |else_| {
    ///             else_.i32_const(34);
    ///         },
    ///     );
    /// ```
    pub fn if_else_at(
        &mut self,
        position: usize,
        ty: impl Into<InstrSeqType>,
        consequent: impl FnOnce(&mut InstrSeqBuilder),
        alternative: impl FnOnce(&mut InstrSeqBuilder),
    ) -> &mut Self {
        let ty = ty.into();

        let consequent = {
            let mut builder = self.dangling_instr_seq(ty);
            consequent(&mut builder);
            builder.id
        };

        let alternative = {
            let mut builder = self.dangling_instr_seq(ty);
            alternative(&mut builder);
            builder.id
        };

        self.instr_at(
            position,
            IfElse {
                consequent,
                alternative,
            },
        )
    }
}

impl Deref for InstrSeqBuilder<'_> {
    type Target = FunctionBuilder;

    fn deref(&self) -> &FunctionBuilder {
        &*self.builder
    }
}

impl DerefMut for InstrSeqBuilder<'_> {
    fn deref_mut(&mut self) -> &mut FunctionBuilder {
        &mut *self.builder
    }
}
