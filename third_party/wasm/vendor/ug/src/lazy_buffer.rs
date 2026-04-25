use crate::{bail, Const, DType, Device, Dim, Result, Shape};
use std::cell::RefCell;
use std::sync::Arc;

type Callback<S> = Arc<dyn Fn(Vec<&mut S>) -> Result<()>>;
pub struct CustomF<S>(Callback<S>);

impl<S> std::fmt::Debug for CustomF<S> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "<func>")
    }
}

impl<S> std::ops::Deref for CustomF<S> {
    type Target = Arc<dyn Fn(Vec<&mut S>) -> Result<()>>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl<S> Clone for CustomF<S> {
    fn clone(&self) -> Self {
        Self(self.0.clone())
    }
}

impl<S> CustomF<S> {
    pub fn new<F: 'static + Fn(Vec<&mut S>) -> Result<()>>(f: F) -> Self {
        Self(Arc::new(f))
    }
}

/// Unique identifier for LazyBuffer.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Id(usize);

impl Id {
    fn new() -> Self {
        // https://users.rust-lang.org/t/idiomatic-rust-way-to-generate-unique-id/33805
        use std::sync::atomic;
        static COUNTER: atomic::AtomicUsize = atomic::AtomicUsize::new(1);
        Self(COUNTER.fetch_add(1, atomic::Ordering::Relaxed))
    }

    pub fn as_usize(&self) -> usize {
        self.0
    }
}

#[derive(Debug)]
pub enum Op<D: Device> {
    Unary(crate::lang::UnaryOp, LazyBuffer<D>),
    Binary(crate::lang::BinaryOp, LazyBuffer<D>, LazyBuffer<D>),
    MatMul(LazyBuffer<D>, LazyBuffer<D>, (usize, usize, usize, usize), bool),
    Reduce(crate::lang::ReduceOp, LazyBuffer<D>, usize),
    Const(crate::lang::Const),
    /// Values are leafs of the op DAG, holding values that do not depend on other values.
    Value,
    Layout(crate::lang::op::LayoutOp, LazyBuffer<D>),
    Reshape(LazyBuffer<D>),
    Custom {
        f: CustomF<D::Slice>,
        args: Vec<LazyBuffer<D>>,
    },
    /// In-place variant of the custom op.
    CustomIp {
        f: CustomF<D::Slice>,
        args: Vec<LazyBuffer<D>>,
        src: LazyBuffer<D>,
    },
    /// Custom op based on some SSA code.
    Ssa {
        ssa: crate::lang::ssa::Kernel,
        args: Vec<LazyBuffer<D>>,
    },
    /// Set the data from src using data from values. The LazyBuffer is shared between self and
    /// src.
    Set {
        values: LazyBuffer<D>,
        src: LazyBuffer<D>,
        dst_layout: crate::Layout,
    },
}

pub struct LazyBuffer<D: Device>(Arc<LazyBufferInner<D>>);

impl<D: Device> Clone for LazyBuffer<D> {
    fn clone(&self) -> Self {
        Self(self.0.clone())
    }
}

impl<D: Device> std::ops::Deref for LazyBuffer<D> {
    type Target = LazyBufferInner<D>;

    fn deref(&self) -> &Self::Target {
        self.0.as_ref()
    }
}

pub struct LazyBufferInner<D: Device> {
    id: Id,
    // The RefCell here could lead to dynamic borrow check failures when LazyBuffers are shared
    // between threads, if this happens one should use some mutex to protect the underlying
    // data containers.
    data: Arc<RefCell<Option<D::Slice>>>,
    op: Op<D>,
    dtype: crate::DType,
    /// The shape for the buffer, the buffer always uses a C style memory layout.
    shape: Shape,
    device: D,
}

impl<D: Device> LazyBuffer<D> {
    #[allow(clippy::missing_safety_doc)]
    pub unsafe fn maybe_allocate_uninit(&self) -> Result<()> {
        let mut data = self.data.try_borrow_mut()?;
        if data.is_none() {
            let nels = self.shape.num_elements();
            let v = self.device.allocate_uninit(self.dtype, nels)?;
            *data = Some(v)
        }
        Ok(())
    }

    pub fn op(&self) -> &Op<D> {
        &self.op
    }

    pub fn in_place_op(&self) -> bool {
        match self.op {
            Op::CustomIp { .. } | Op::Set { .. } => true,
            Op::Ssa { .. }
            | Op::Unary(_, _)
            | Op::Const(_)
            | Op::Value
            | Op::Binary(_, _, _)
            | Op::MatMul(_, _, _, _)
            | Op::Reduce(_, _, _)
            | Op::Layout(_, _)
            | Op::Reshape(_)
            | Op::Custom { .. } => false,
        }
    }

    pub fn realized(&self) -> Result<bool> {
        // TODO: Always re-running in place ops is odd, we should keep track of whether
        // this has run or not in the past to know if we want to run it again.
        if self.in_place_op() {
            return Ok(false);
        }
        let data = self.data.try_borrow()?;
        Ok(data.is_some())
    }

    pub fn realize(&self) -> Result<()> {
        if self.realized()? {
            return Ok(());
        }
        let schedule = crate::Schedule::create_one(self)?;
        let schedule = schedule.compile()?;
        schedule.run()?;
        Ok(())
    }

    pub fn data(&self) -> &RefCell<Option<D::Slice>> {
        &self.data
    }

    pub fn data_vec<DT: crate::WithDType>(&self) -> Result<Vec<DT>> {
        use crate::Slice;

        self.realize()?;
        let data = self.data.as_ref().try_borrow()?;
        match data.as_ref() {
            None => bail!("no data"),
            Some(data) => {
                let mut vs = vec![DT::zero(); self.shape.num_elements()];
                D::Slice::copy_device_to_host(data, &mut vs)?;
                Ok(vs)
            }
        }
    }

    pub fn device(&self) -> &D {
        &self.device
    }

    pub fn dtype(&self) -> DType {
        self.dtype
    }

    pub fn shape(&self) -> &Shape {
        &self.shape
    }

    pub fn elem_count(&self) -> usize {
        self.shape.elem_count()
    }

    pub fn dims(&self) -> &[usize] {
        self.shape().dims()
    }

    pub fn dims0(&self) -> Result<()> {
        self.shape().dims0()
    }

    pub fn dims1(&self) -> Result<usize> {
        self.shape().dims1()
    }

    pub fn dims2(&self) -> Result<(usize, usize)> {
        self.shape().dims2()
    }

    pub fn dims3(&self) -> Result<(usize, usize, usize)> {
        self.shape().dims3()
    }

    pub fn dims4(&self) -> Result<(usize, usize, usize, usize)> {
        self.shape().dims4()
    }

    pub fn rank(&self) -> usize {
        self.shape().rank()
    }

    pub fn unary(&self, op: crate::lang::UnaryOp) -> Result<Self> {
        // TODO: dtype/op checks.
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Unary(op, self.clone()),
            dtype: self.dtype,
            shape: self.shape.clone(),
            device: self.device.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn cast(&self, dtype: DType) -> Result<Self> {
        if self.dtype == dtype {
            return Ok(self.clone());
        }
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Unary(crate::lang::UnaryOp::Cast(dtype), self.clone()),
            dtype,
            shape: self.shape.clone(),
            device: self.device.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn binary(&self, op: crate::lang::BinaryOp, rhs: Self) -> Result<Self> {
        if self.shape != rhs.shape {
            bail!("shape mismatch in {op:?}, {:?} vs {:?}", self.shape, rhs.shape)
        }
        if self.dtype != rhs.dtype {
            bail!("dtype mismatch in {op:?}, {:?} vs {:?}", self.dtype, rhs.dtype)
        }
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Binary(op, self.clone(), rhs),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: self.shape.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    // TODO: Should this be marked as unsafe?
    pub fn alloc_uninit<S: Into<Shape>>(dtype: DType, s: S, device: &D) -> Result<Self> {
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Value,
            dtype,
            device: device.clone(),
            shape: s.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn custom<F: 'static + Fn(Vec<&mut D::Slice>) -> Result<()>, S: Into<Shape>>(
        f: F,
        args: Vec<Self>,
        s: S,
        dtype: DType,
        device: &D,
    ) -> Result<Self> {
        let f = CustomF::new(f);
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Custom { f, args },
            dtype,
            device: device.clone(),
            shape: s.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    /// Applies a custom in-place operation.
    pub fn custom_ip<F: 'static + Fn(Vec<&mut D::Slice>) -> Result<()>>(
        &self,
        f: F,
        args: Vec<Self>,
    ) -> Result<Self> {
        let f = CustomF::new(f);
        let inner = LazyBufferInner {
            id: Id::new(),
            data: self.data.clone(),
            op: Op::CustomIp { f, args, src: self.clone() },
            dtype: self.dtype,
            device: self.device.clone(),
            shape: self.shape.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    /// Sets self using data from values.
    pub fn set(&self, values: Self) -> Result<Self> {
        if self.shape != values.shape {
            bail!("shape mismatch in set, {:?} vs {:?}", self.shape, values.shape)
        }
        if self.dtype != values.dtype {
            bail!("dtype mismatch in set, {:?} vs {:?}", self.dtype, values.dtype)
        }
        let dst_layout = crate::Layout::from_shape(self.shape());
        let inner = LazyBufferInner {
            id: Id::new(),
            data: self.data.clone(),
            op: Op::Set { values, src: self.clone(), dst_layout },
            dtype: self.dtype,
            device: self.device.clone(),
            shape: self.shape.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn set_l(&self, values: Self, dst_layout: crate::Layout) -> Result<Self> {
        let max_offset = dst_layout
            .dims()
            .iter()
            .zip(dst_layout.strides().iter())
            .map(|(d, s)| d.saturating_sub(1) * s)
            .sum::<usize>()
            + dst_layout.offset();
        if max_offset >= self.shape().elem_count() {
            bail!(
                "set_l out of bounds, dst shape {:?}, layout {dst_layout:?}, max off {max_offset}",
                self.shape()
            )
        }
        if dst_layout.num_elements() != values.shape().elem_count() {
            bail!(
                "set_l mismatch between the values shape {:?} and layout {dst_layout:?}",
                values.shape()
            )
        }
        if self.dtype != values.dtype {
            bail!("dtype mismatch in set, {:?} vs {:?}", self.dtype, values.dtype)
        }
        let inner = LazyBufferInner {
            id: Id::new(),
            data: self.data.clone(),
            op: Op::Set { values, src: self.clone(), dst_layout },
            dtype: self.dtype,
            device: self.device.clone(),
            shape: self.shape.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn ssa<S: Into<Shape>>(
        ssa: crate::lang::ssa::Kernel,
        args: Vec<Self>,
        s: S,
        dtype: DType,
        device: &D,
    ) -> Result<Self> {
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Ssa { ssa, args },
            dtype,
            device: device.clone(),
            shape: s.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn matmul(&self, rhs: Self) -> Result<Self> {
        self.matmul_(rhs, false)
    }

    pub fn matmul_t(&self, rhs: Self) -> Result<Self> {
        self.matmul_(rhs, true)
    }

    pub fn matmul_(&self, rhs: Self, transpose: bool) -> Result<Self> {
        let lhs_dims = self.dims();
        let rhs_dims = rhs.dims();
        let dim = lhs_dims.len();
        let rdim = rhs_dims.len();

        if dim < 2 || rdim < 2 {
            bail!("shape mismatch in matmul {lhs_dims:?} {rhs_dims:?}")
        }

        let m = lhs_dims[dim - 2];
        let k = lhs_dims[dim - 1];
        let (k2, n) = if transpose {
            (rhs_dims[rdim - 1], rhs_dims[rdim - 2])
        } else {
            (rhs_dims[rdim - 2], rhs_dims[rdim - 1])
        };

        let lhs_bsz: usize = lhs_dims[..dim - 2].iter().product();
        let rhs_bsz: usize = rhs_dims[..rdim - 2].iter().product();
        if k != k2 || lhs_bsz != rhs_bsz {
            bail!("shape mismatch in matmul {lhs_dims:?} {rhs_dims:?}")
        }
        let bmnk = (lhs_bsz, m, n, k);
        let mut shape = lhs_dims[..dim - 2].to_vec();
        shape.push(m);
        shape.push(n);
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::MatMul(self.clone(), rhs, bmnk, transpose),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: shape.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn reduce<I: Dim>(&self, op: crate::lang::ReduceOp, dim: I) -> Result<Self> {
        // TODO: dtype/op checks.
        let shape = self.shape(); // TODO: squeeze or remove axis.
        let dim = dim.to_index(shape, "reduce")?;
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Reduce(op, self.clone(), dim),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: shape.clone(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn id(&self) -> Id {
        self.id
    }

    pub fn split_dim<I: Dim>(&self, dim: I, size1: usize, size2: usize) -> Result<Self> {
        use crate::lang::op::LayoutOp::SplitDim;
        let dim = dim.to_index(self.shape(), "split_dim")?;
        let mut dims = self.dims().to_vec();
        let size = dims.remove(dim);
        if size1 * size2 != size {
            bail!("unexpected target sizes for split_dim {dim}, {size1}x{size2} != {size}",)
        }
        dims.insert(dim, size2);
        dims.insert(dim, size1);
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Layout(SplitDim { dim, lhs: dim, rhs: dim + 1 }, self.clone()),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: dims.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    /// Merge the dims dim and dim + 1 together.
    pub fn merge_dims<I: Dim>(&self, dim: I) -> Result<Self> {
        use crate::lang::op::LayoutOp::MergeDims;
        let dim = dim.to_index(self.shape(), "split_dim")?;
        if dim + 1 >= self.rank() {
            bail!("unexpected dim for merge_dims {dim} {:?}", self.shape())
        }
        let mut dims = self.dims().to_vec();
        let size_p = dims.remove(dim + 1);
        dims[dim] *= size_p;
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Layout(MergeDims { dim, lhs: dim, rhs: dim + 1 }, self.clone()),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: dims.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    // Contrary to pytorch, this returns an error if the target dimension does not have
    // a size 1.
    pub fn squeeze<D1: Dim>(&self, dim: D1) -> Result<Self> {
        let dim = dim.to_index(self.shape(), "squeeze")?;
        let mut dims = self.dims().to_vec();
        if dims[dim] != 1 {
            bail!("cannot squeeze on dim {dim} for shape {dims:?}")
        }
        dims.remove(dim);
        self.reshape(dims)
    }

    pub fn get_on_dim<D1: Dim>(&self, dim: D1, index: usize) -> Result<Self> {
        let dim = dim.to_index(self.shape(), "get")?;
        let narrowed = self.narrow(dim, index, 1)?;
        narrowed.squeeze(dim)
    }

    pub fn get(&self, index: usize) -> Result<Self> {
        self.get_on_dim(0, index)
    }

    pub fn unsqueeze<D1: Dim>(&self, dim: D1) -> Result<Self> {
        let mut dims = self.dims().to_vec();
        let dim = dim.to_index_plus_one(self.shape(), "unsqueeze")?;
        dims.insert(dim, 1);
        self.reshape(dims)
    }

    pub fn reshape<S: Into<Shape>>(&self, s: S) -> Result<Self> {
        let s: Shape = s.into();
        let dst_nel = s.num_elements();
        let src_nel = self.shape().num_elements();
        if dst_nel != src_nel {
            bail!(
                "cannot reshape between {:?} ({src_nel} elts) and {s:?} ({dst_nel} elts)",
                self.shape()
            )
        }
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Reshape(self.clone()),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: s,
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn narrow<D1: Dim>(&self, dim: D1, offset: usize, length: usize) -> Result<Self> {
        let dim = dim.to_index(self.shape(), "narrow")?;
        let size = self.dims()[dim];
        if offset + length > size {
            bail!("cannot narrow, offset {offset} + length {length} > size {dim}")
        }
        if offset == 0 && length == size {
            return Ok(self.clone());
        }
        let mut dims = self.dims().to_vec();
        dims[dim] = length;
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Layout(crate::lang::op::LayoutOp::Narrow { dim, offset }, self.clone()),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: dims.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn broadcast<S: Into<Shape>>(&self, s: S) -> Result<Self> {
        let s: Shape = s.into();
        if self.shape().rank() > s.rank() {
            bail!("target shape {s:?} has less dimensions than original shape {:?}", self.shape())
        }
        let inserted_dims = s.rank() - self.shape().rank();
        let mut broadcasted_dims = (0..inserted_dims).collect::<Vec<_>>();
        for (dim_idx, &dim_len) in self.shape.dims().iter().enumerate() {
            let dim_idx = dim_idx + inserted_dims;
            if s.dims()[dim_idx] != dim_len {
                if dim_len == 1 {
                    broadcasted_dims.push(dim_idx)
                } else {
                    bail!("cannot broadcast from {:?} to {s:?}", self.shape)
                }
            }
        }
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Layout(
                crate::lang::op::LayoutOp::Broadcast { inserted_dims, broadcasted_dims },
                self.clone(),
            ),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: s,
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn transpose<D1: Dim, D2: Dim>(&self, dim1: D1, dim2: D2) -> Result<Self> {
        let dim1 = dim1.to_index(self.shape(), "transpose")?;
        let dim2 = dim2.to_index(self.shape(), "transpose")?;
        if dim1 == dim2 {
            return Ok(self.clone());
        }
        let mut dims = self.dims().to_vec();
        dims.swap(dim1, dim2);
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Layout(crate::lang::op::LayoutOp::Transpose { dim1, dim2 }, self.clone()),
            dtype: self.dtype,
            device: self.device.clone(),
            shape: dims.into(),
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn cst<C: TryInto<Const> + std::fmt::Debug + Copy, S: Into<Shape>>(
        c: C,
        s: S,
        device: &D,
    ) -> Result<Self> {
        let c: Const = match c.try_into() {
            Err(_) => bail!("unable to create const for {c:?}"),
            Ok(v) => v,
        };
        let s: Shape = s.into();
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(None)),
            op: Op::Const(c),
            dtype: c.dtype(),
            device: device.clone(),
            shape: s,
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn from_slice<S: Into<Shape>>(data: D::Slice, s: S) -> Result<Self> {
        use crate::Slice;

        let s: Shape = s.into();
        let device = data.device().clone();
        let dtype = data.dtype();
        if s.num_elements() != data.len() {
            bail!("unexpected number of elements {} for shape {s:?}", data.len())
        }
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(Some(data))),
            // We don't keep a hold on the Copy data here so as to reduce memory usage.
            op: Op::Value,
            dtype,
            device: device.clone(),
            shape: s,
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn copy<'a, R: Into<crate::CpuStorageRef<'a>>, S: Into<Shape>>(
        data: R,
        s: S,
        device: &D,
    ) -> Result<Self> {
        use crate::CpuStorageRef as C;
        use crate::Slice;

        let data: crate::CpuStorageRef<'a> = data.into();
        let s: Shape = s.into();
        if s.num_elements() != data.len() {
            bail!("unexpected number of elements {} for shape {s:?}", data.len())
        }
        let dtype = data.dtype();
        let mut slice = unsafe { device.allocate_uninit(dtype, data.len()) }?;
        match data {
            C::BF16(data) => slice.copy_host_to_device(data)?,
            C::F16(data) => slice.copy_host_to_device(data)?,
            C::F32(data) => slice.copy_host_to_device(data)?,
            C::I32(data) => slice.copy_host_to_device(data)?,
            C::I64(data) => slice.copy_host_to_device(data)?,
        };
        let inner = LazyBufferInner {
            id: Id::new(),
            data: Arc::new(RefCell::new(Some(slice))),
            // We don't keep a hold on the Copy data here so as to reduce memory usage.
            op: Op::Value,
            dtype,
            device: device.clone(),
            shape: s,
        };
        let lb = LazyBuffer(Arc::new(inner));
        Ok(lb)
    }

    pub fn cat<D1: Dim>(args: &[&Self], dim: D1) -> Result<Self> {
        if args.is_empty() {
            bail!("empty list in cat")
        }
        let arg0 = args[0];
        if args.len() == 1 {
            return Ok(arg0.clone());
        }
        let dim = dim.to_index(arg0.shape(), "cat")?;
        let mut dims = arg0.dims().to_vec();
        dims[dim] = 0;
        for (arg_idx, arg) in args.iter().enumerate() {
            if arg.dtype() != arg0.dtype() {
                let shapes: Vec<_> = args.iter().map(|a| a.dtype()).collect();
                bail!("mismatch between dtypes in cat, shapes: {shapes:?}")
            }
            if arg.rank() != arg0.rank() {
                let shapes: Vec<_> = args.iter().map(|a| a.shape()).collect();
                bail!("mismatch between ranks in cat, shapes: {shapes:?}")
            }
            dims[dim] += arg.dims()[dim];
            for (dim_idx, (v1, v2)) in dims.iter().zip(arg.shape().dims().iter()).enumerate() {
                if dim_idx != dim && v1 != v2 {
                    bail!(
                        "mismatch between shapes in cat on dim {dim}, arg0: {:?}, arg{arg_idx}: {:?}",
                        arg0.shape(),
                        arg.shape()
                    )
                }
            }
        }
        let mut stride = 1;
        for d in &dims[dim + 1..] {
            stride *= d
        }
        let mut offset = 0;
        let mut vs = Self::alloc_uninit(arg0.dtype, dims, arg0.device())?;
        for &arg in args.iter() {
            let mut dst_layout = crate::Layout::from_shape(arg.shape());
            dst_layout.set_offset(offset * stride);
            vs = vs.set_l(arg.clone(), dst_layout)?;
            offset += arg.dims()[dim]
        }
        Ok(vs)
    }
}
