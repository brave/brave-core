use crate::{DType, Device, Error, Result, Shape, Slice, WithDType};
use safetensors::tensor as st;
use safetensors::tensor::SafeTensors;
use std::collections::HashMap;
use std::path::Path;

#[derive(yoke::Yokeable)]
struct SafeTensors_<'a>(SafeTensors<'a>);

pub struct MmapedSafetensors {
    safetensors: Vec<yoke::Yoke<SafeTensors_<'static>, memmap2::Mmap>>,
    routing: Option<HashMap<String, usize>>,
}

impl MmapedSafetensors {
    /// Creates a wrapper around a memory mapped file and deserialize the safetensors header.
    ///
    /// # Safety
    ///
    /// The unsafe is inherited from [`memmap2::MmapOptions`].
    pub unsafe fn new<P: AsRef<Path>>(p: P) -> Result<Self> {
        let p = p.as_ref();
        let file = std::fs::File::open(p).map_err(|e| Error::from(e).with_path(p))?;
        let file =
            memmap2::MmapOptions::new().map(&file).map_err(|e| Error::from(e).with_path(p))?;
        let safetensors = yoke::Yoke::<SafeTensors_<'static>, memmap2::Mmap>::try_attach_to_cart(
            file,
            |data: &[u8]| {
                let st = safetensors::SafeTensors::deserialize(data)
                    .map_err(|e| Error::from(e).with_path(p))?;
                Ok::<_, Error>(SafeTensors_(st))
            },
        )?;
        Ok(Self { safetensors: vec![safetensors], routing: None })
    }

    /// Creates a wrapper around multiple memory mapped file and deserialize the safetensors headers.
    ///
    /// If a tensor name appears in multiple files, the last entry is returned.
    ///
    /// # Safety
    ///
    /// The unsafe is inherited from [`memmap2::MmapOptions`].
    pub unsafe fn multi<P: AsRef<Path>>(paths: &[P]) -> Result<Self> {
        let mut routing = HashMap::new();
        let mut safetensors = vec![];
        for (index, p) in paths.iter().enumerate() {
            let p = p.as_ref();
            let file = std::fs::File::open(p).map_err(|e| Error::from(e).with_path(p))?;
            let file =
                memmap2::MmapOptions::new().map(&file).map_err(|e| Error::from(e).with_path(p))?;
            let data = yoke::Yoke::<SafeTensors_<'static>, memmap2::Mmap>::try_attach_to_cart(
                file,
                |data: &[u8]| {
                    let st = safetensors::SafeTensors::deserialize(data)
                        .map_err(|e| Error::from(e).with_path(p))?;
                    Ok::<_, Error>(SafeTensors_(st))
                },
            )?;
            for k in data.get().0.names() {
                routing.insert(k.to_string(), index);
            }
            safetensors.push(data)
        }
        Ok(Self { safetensors, routing: Some(routing) })
    }

    pub fn load<D: Device>(&self, name: &str, device: &D) -> Result<crate::LazyBuffer<D>> {
        let (shape, slice) = self.load_slice(name, device)?;
        crate::LazyBuffer::from_slice(slice, shape)
    }

    pub fn load_with_cast<D: Device>(
        &self,
        name: &str,
        dtype: DType,
        device: &D,
    ) -> Result<crate::LazyBuffer<D>> {
        let (shape, slice) = self.load_slice_with_cast(name, dtype, device)?;
        crate::LazyBuffer::from_slice(slice, shape)
    }

    pub fn load_slice<D: Device>(&self, name: &str, device: &D) -> Result<(Shape, D::Slice)> {
        let view = self.get(name)?;
        let shape: Shape = view.shape().into();
        let dtype = match view.dtype() {
            st::Dtype::BF16 => DType::BF16,
            st::Dtype::F16 => DType::F16,
            st::Dtype::F32 => DType::F32,
            st::Dtype::I32 => DType::I32,
            st::Dtype::I64 => DType::I64,
            dtype => crate::bail!("unsupported dtype for {name}: {dtype:?}"),
        };
        let mut slice = unsafe { device.allocate_uninit(dtype, shape.num_elements()) }?;
        match dtype {
            DType::F16 => {
                let data = convert_slice::<half::f16>(view.data());
                slice.copy_host_to_device(&data)?
            }
            DType::BF16 => {
                let data = convert_slice::<half::bf16>(view.data());
                slice.copy_host_to_device(&data)?
            }
            DType::F32 => {
                let data = convert_slice::<f32>(view.data());
                slice.copy_host_to_device(&data)?
            }
            DType::I32 => {
                let data = convert_slice::<i32>(view.data());
                slice.copy_host_to_device(&data)?
            }
            DType::I64 => {
                let data = convert_slice::<i64>(view.data());
                slice.copy_host_to_device(&data)?
            }
        };
        Ok((shape, slice))
    }

    pub fn load_slice_with_cast<D: Device>(
        &self,
        name: &str,
        dst_dtype: DType,
        device: &D,
    ) -> Result<(Shape, D::Slice)> {
        let view = self.get(name)?;
        let shape: Shape = view.shape().into();
        let src_dtype = match view.dtype() {
            st::Dtype::BF16 => DType::BF16,
            st::Dtype::F16 => DType::F16,
            st::Dtype::F32 => DType::F32,
            st::Dtype::I32 => DType::I32,
            st::Dtype::I64 => DType::I64,
            dtype => crate::bail!("unsupported dtype for {name}: {dtype:?}"),
        };
        if dst_dtype == src_dtype {
            return self.load_slice(name, device);
        }
        let mut slice = unsafe { device.allocate_uninit(dst_dtype, shape.num_elements()) }?;
        // The pattern matching below is overly verbose but new dtypes are not added that often
        match (src_dtype, dst_dtype) {
            // BF16 convs
            (DType::BF16, DType::BF16) => {
                let data = convert_slice::<half::bf16>(view.data());
                slice.copy_host_to_device(&data)?
            }
            (DType::BF16, DType::F16) => {
                let data = convert_slice::<half::bf16>(view.data());
                let data: Vec<_> =
                    data.iter().map(|v| half::f16::from_f32(f32::from(*v))).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::BF16, DType::F32) => {
                let data = convert_slice::<half::bf16>(view.data());
                let data: Vec<_> = data.iter().map(|v| f32::from(*v)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::BF16, DType::I32) => {
                let data = convert_slice::<half::bf16>(view.data());
                let data: Vec<_> = data.iter().map(|v| f32::from(*v) as i32).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::BF16, DType::I64) => {
                let data = convert_slice::<half::bf16>(view.data());
                let data: Vec<_> = data.iter().map(|v| f32::from(*v) as i32).collect();
                slice.copy_host_to_device(&data)?
            }
            // F16 convs
            (DType::F16, DType::BF16) => {
                let data = convert_slice::<half::f16>(view.data());
                let data: Vec<_> =
                    data.iter().map(|v| half::bf16::from_f32(f32::from(*v))).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::F16, DType::F16) => {
                let data = convert_slice::<half::f16>(view.data());
                slice.copy_host_to_device(&data)?
            }
            (DType::F16, DType::F32) => {
                let data = convert_slice::<half::f16>(view.data());
                let data: Vec<_> = data.iter().map(|v| f32::from(*v)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::F16, DType::I32) => {
                let data = convert_slice::<half::f16>(view.data());
                let data: Vec<_> = data.iter().map(|v| f32::from(*v) as i32).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::F16, DType::I64) => {
                let data = convert_slice::<half::f16>(view.data());
                let data: Vec<_> = data.iter().map(|v| f32::from(*v) as i32).collect();
                slice.copy_host_to_device(&data)?
            }
            // F32 convs
            (DType::F32, DType::BF16) => {
                let data = convert_slice::<f32>(view.data());
                let data: Vec<_> = data.iter().map(|v| half::bf16::from_f32(*v)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::F32, DType::F16) => {
                let data = convert_slice::<f32>(view.data());
                let data: Vec<_> = data.iter().map(|v| half::f16::from_f32(*v)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::F32, DType::F32) => {
                let data = convert_slice::<f32>(view.data());
                slice.copy_host_to_device(&data)?
            }
            (DType::F32, DType::I32) => {
                let data = convert_slice::<f32>(view.data());
                let data: Vec<_> = data.iter().map(|v| *v as i32).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::F32, DType::I64) => {
                let data = convert_slice::<f32>(view.data());
                let data: Vec<_> = data.iter().map(|v| *v as i64).collect();
                slice.copy_host_to_device(&data)?
            }
            // I32 convs
            (DType::I32, DType::BF16) => {
                let data = convert_slice::<i32>(view.data());
                let data: Vec<_> = data.iter().map(|v| half::bf16::from_f32(*v as f32)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I32, DType::F16) => {
                let data = convert_slice::<i32>(view.data());
                let data: Vec<_> = data.iter().map(|v| half::f16::from_f32(*v as f32)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I32, DType::F32) => {
                let data = convert_slice::<i32>(view.data());
                let data: Vec<_> = data.iter().map(|v| *v as f32).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I32, DType::I32) => {
                let data = convert_slice::<i32>(view.data());
                slice.copy_host_to_device(&data)?
            }
            (DType::I32, DType::I64) => {
                let data = convert_slice::<i32>(view.data());
                let data: Vec<_> = data.iter().map(|v| *v as i64).collect();
                slice.copy_host_to_device(&data)?
            }
            // I64 convs
            (DType::I64, DType::BF16) => {
                let data = convert_slice::<i64>(view.data());
                let data: Vec<_> = data.iter().map(|v| half::bf16::from_f32(*v as f32)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I64, DType::F16) => {
                let data = convert_slice::<i64>(view.data());
                let data: Vec<_> = data.iter().map(|v| half::f16::from_f32(*v as f32)).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I64, DType::F32) => {
                let data = convert_slice::<i64>(view.data());
                let data: Vec<_> = data.iter().map(|v| *v as f32).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I64, DType::I32) => {
                let data = convert_slice::<i64>(view.data());
                let data: Vec<_> = data.iter().map(|v| *v as i32).collect();
                slice.copy_host_to_device(&data)?
            }
            (DType::I64, DType::I64) => {
                let data = convert_slice::<i64>(view.data());
                slice.copy_host_to_device(&data)?
            }
        };
        Ok((shape, slice))
    }

    pub fn tensors(&self) -> Vec<(String, st::TensorView<'_>)> {
        let mut tensors = vec![];
        for safetensors in self.safetensors.iter() {
            tensors.push(safetensors.get().0.tensors())
        }
        tensors.into_iter().flatten().collect()
    }

    pub fn get(&self, name: &str) -> Result<st::TensorView<'_>> {
        let index = match &self.routing {
            None => 0,
            Some(routing) => {
                let index = routing
                    .get(name)
                    .ok_or_else(|| Error::CannotFindTensor { path: name.to_string() }.bt())?;
                *index
            }
        };
        Ok(self.safetensors[index].get().0.tensor(name)?)
    }
}

pub fn convert_slice<T: WithDType>(data: &[u8]) -> std::borrow::Cow<'_, [T]> {
    let size_in_bytes = T::DTYPE.size_in_bytes();
    let elem_count = data.len() / size_in_bytes;
    if (data.as_ptr() as usize) % size_in_bytes == 0 {
        // SAFETY This is safe because we just checked that this
        // was correctly aligned.
        let data: &[T] =
            unsafe { std::slice::from_raw_parts(data.as_ptr() as *const T, elem_count) };
        std::borrow::Cow::Borrowed(data)
    } else {
        // XXX: We need to specify `T` here, otherwise the compiler will infer u8 because of the following cast
        // Making this vector too small to fit a full f16/f32/f64 weights, resulting in out-of-bounds access
        let mut c: Vec<T> = Vec::with_capacity(elem_count);
        // SAFETY: We just created c, so the allocated memory is necessarily
        // contiguous and non overlapping with the view's data.
        // We're downgrading the `c` pointer from T to u8, which removes alignment
        // constraints.
        unsafe {
            std::ptr::copy_nonoverlapping(data.as_ptr(), c.as_mut_ptr() as *mut u8, data.len());
            c.set_len(elem_count)
        }
        std::borrow::Cow::Owned(c)
    }
}
