(module
 (type $i32_=>_i32 (func (param i32) (result i32)))
 (type $i32_i32_=>_none (func (param i32 i32)))
 (type $none_=>_none (func))
 (type $i32_i32_=>_i32 (func (param i32 i32) (result i32)))
 (type $i32_i32_i32_=>_none (func (param i32 i32 i32)))
 (type $i32_=>_none (func (param i32)))
 (type $i32_i32_i32_=>_i32 (func (param i32 i32 i32) (result i32)))
 (type $none_=>_i32 (func (result i32)))
 (type $i32_i32_i32_i32_=>_i32 (func (param i32 i32 i32 i32) (result i32)))
 (type $i32_i32_i32_i32_i32_=>_i32 (func (param i32 i32 i32 i32 i32) (result i32)))
 (import "__wbindgen_placeholder__" "__wbindgen_describe" (func $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E (param i32)))
 (import "__wbindgen_placeholder__" "__wbg_alert_f30b78c50df83b2d" (func $_ZN11hello_world5alert28__wbg_alert_f30b78c50df83b2d17he1e4a4b8d8d0ea44E (param i32 i32)))
 (import "__wbindgen_externref_xform__" "__wbindgen_externref_table_grow" (func $_ZN12wasm_bindgen9externref31__wbindgen_externref_table_grow17ha4f746c989afa1c2E (param i32) (result i32)))
 (import "__wbindgen_externref_xform__" "__wbindgen_externref_table_set_null" (func $_ZN12wasm_bindgen9externref35__wbindgen_externref_table_set_null17h958e92ab89f726f8E (param i32)))
 (global $__stack_pointer (mut i32) (i32.const 1048576))
 (global $global$1 i32 (i32.const 1049076))
 (global $global$2 i32 (i32.const 1049088))
 (memory $0 17)
 (data $.rodata (i32.const 1048576) "Hello, world!")
 (table $0 1 1 funcref)
 (export "memory" (memory $0))
 (export "__wbindgen_describe___wbg_alert_f30b78c50df83b2d" (func $__wbindgen_describe___wbg_alert_f30b78c50df83b2d))
 (export "greet" (func $greet))
 (export "__wbindgen_describe_greet" (func $__wbindgen_describe_greet))
 (export "__wbindgen_exn_store" (func $__wbindgen_exn_store))
 (export "__wbindgen_malloc" (func $__wbindgen_malloc))
 (export "__wbindgen_realloc" (func $__wbindgen_realloc))
 (export "__wbindgen_free" (func $__wbindgen_free))
 (export "__externref_table_alloc" (func $__externref_table_alloc))
 (export "__externref_table_dealloc" (func $__externref_table_dealloc))
 (export "__externref_drop_slice" (func $__externref_drop_slice))
 (export "__externref_heap_live_count" (func $__externref_heap_live_count))
 (export "__data_end" (global $global$1))
 (export "__heap_base" (global $global$2))
 (func $__wbindgen_describe___wbg_alert_f30b78c50df83b2d
  (call $_ZN12wasm_bindgen4__rt19link_mem_intrinsics17hdfca264069c2a983E)
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 11)
  )
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 0)
  )
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 1)
  )
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 15)
  )
  (call $_ZN60_$LT$str$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h75ce916392c06b84E)
  (call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E)
  (call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E)
 )
 (func $greet
  (call $_ZN11hello_world5alert28__wbg_alert_f30b78c50df83b2d17he1e4a4b8d8d0ea44E
   (i32.const 1048576)
   (i32.const 13)
  )
 )
 (func $__wbindgen_describe_greet
  (call $_ZN12wasm_bindgen4__rt19link_mem_intrinsics17hdfca264069c2a983E)
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 11)
  )
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 0)
  )
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 0)
  )
  (call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E)
  (call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E)
 )
 (func $__rust_alloc (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local.set $2
   (call $__rdl_alloc
    (local.get $0)
    (local.get $1)
   )
  )
  (return
   (local.get $2)
  )
 )
 (func $__rust_dealloc (param $0 i32) (param $1 i32) (param $2 i32)
  (call $__rdl_dealloc
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
  (return)
 )
 (func $__rust_realloc (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (result i32)
  (local $4 i32)
  (local.set $4
   (call $__rdl_realloc
    (local.get $0)
    (local.get $1)
    (local.get $2)
    (local.get $3)
   )
  )
  (return
   (local.get $4)
  )
 )
 (func $_ZN12wasm_bindgen4__rt19link_mem_intrinsics17hdfca264069c2a983E
  (call $_ZN12wasm_bindgen9externref15link_intrinsics17h746d56ace27b8dbdE)
 )
 (func $__wbindgen_exn_store (param $0 i32)
  (i32.store offset=1048596
   (i32.const 0)
   (local.get $0)
  )
  (i32.store8 offset=1048592
   (i32.const 0)
   (i32.const 1)
  )
 )
 (func $__wbindgen_malloc (param $0 i32) (result i32)
  (block $label$1
   (br_if $label$1
    (i32.gt_u
     (local.get $0)
     (i32.const 2147483644)
    )
   )
   (block $label$2
    (br_if $label$2
     (local.get $0)
    )
    (return
     (i32.const 4)
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $0
      (call $__rust_alloc
       (local.get $0)
       (i32.shl
        (i32.lt_u
         (local.get $0)
         (i32.const 2147483645)
        )
        (i32.const 2)
       )
      )
     )
    )
   )
   (return
    (local.get $0)
   )
  )
  (call $_ZN12wasm_bindgen4__rt14malloc_failure17hd9616718b81cbbb5E)
  (unreachable)
 )
 (func $_ZN12wasm_bindgen4__rt14malloc_failure17hd9616718b81cbbb5E
  (call $_ZN3std7process5abort17hf7c8bef35d3938e7E)
  (unreachable)
 )
 (func $__wbindgen_realloc (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.gt_u
      (local.get $1)
      (i32.const 2147483644)
     )
    )
    (br_if $label$1
     (local.tee $1
      (call $__rust_realloc
       (local.get $0)
       (local.get $1)
       (i32.const 4)
       (local.get $2)
      )
     )
    )
   )
   (call $_ZN12wasm_bindgen4__rt14malloc_failure17hd9616718b81cbbb5E)
   (unreachable)
  )
  (local.get $1)
 )
 (func $__wbindgen_free (param $0 i32) (param $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (call $__rust_dealloc
    (local.get $0)
    (local.get $1)
    (i32.const 4)
   )
  )
 )
 (func $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 26)
  )
 )
 (func $_ZN60_$LT$str$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h75ce916392c06b84E
  (call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
   (i32.const 14)
  )
 )
 (func $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE
  (call $_ZN3std7process5abort17hf7c8bef35d3938e7E)
  (unreachable)
 )
 (func $__externref_table_alloc (result i32)
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (i32.load offset=1048600
       (i32.const 0)
      )
     )
    )
    (local.set $0
     (i32.load offset=1048604
      (i32.const 0)
     )
    )
    (br $label$1)
   )
   (i64.store offset=1048616 align=4
    (i32.const 0)
    (i64.const 0)
   )
   (i64.store offset=1048608 align=4
    (i32.const 0)
    (i64.const 0)
   )
   (i32.store offset=1048600
    (i32.const 0)
    (i32.const 1)
   )
   (local.set $0
    (i32.const 4)
   )
  )
  (i32.store offset=1048604
   (i32.const 0)
   (i32.const 4)
  )
  (local.set $1
   (i32.load offset=1048608
    (i32.const 0)
   )
  )
  (local.set $2
   (i32.load offset=1048612
    (i32.const 0)
   )
  )
  (i64.store offset=1048608 align=4
   (i32.const 0)
   (i64.const 0)
  )
  (local.set $3
   (i32.load offset=1048620
    (i32.const 0)
   )
  )
  (local.set $4
   (i32.load offset=1048616
    (i32.const 0)
   )
  )
  (i64.store offset=1048616 align=4
   (i32.const 0)
   (i64.const 0)
  )
  (block $label$3
   (block $label$4
    (block $label$5
     (br_if $label$5
      (i32.eq
       (local.get $4)
       (local.get $2)
      )
     )
     (local.set $2
      (local.get $2)
     )
     (local.set $5
      (local.get $1)
     )
     (local.set $6
      (local.get $0)
     )
     (br $label$4)
    )
    (block $label$6
     (block $label$7
      (br_if $label$7
       (i32.eq
        (local.get $2)
        (local.get $1)
       )
      )
      (local.set $5
       (local.get $1)
      )
      (local.set $6
       (local.get $0)
      )
      (br $label$6)
     )
     (br_if $label$3
      (i32.eq
       (local.tee $6
        (call $_ZN12wasm_bindgen9externref31__wbindgen_externref_table_grow17ha4f746c989afa1c2E
         (local.tee $5
          (select
           (local.get $1)
           (i32.const 128)
           (i32.gt_u
            (local.get $1)
            (i32.const 128)
           )
          )
         )
        )
       )
       (i32.const -1)
      )
     )
     (block $label$8
      (block $label$9
       (br_if $label$9
        (local.get $3)
       )
       (local.set $3
        (local.get $6)
       )
       (br $label$8)
      )
      (br_if $label$3
       (i32.ne
        (i32.add
         (local.get $3)
         (local.get $1)
        )
        (local.get $6)
       )
      )
     )
     (br_if $label$3
      (i32.gt_u
       (local.tee $6
        (i32.shl
         (local.tee $5
          (i32.add
           (local.get $5)
           (local.get $1)
          )
         )
         (i32.const 2)
        )
       )
       (i32.const 2147483644)
      )
     )
     (br_if $label$3
      (i32.eqz
       (local.tee $6
        (call $__rust_alloc
         (local.get $6)
         (i32.const 4)
        )
       )
      )
     )
     (drop
      (call $memcpy
       (local.get $6)
       (local.get $0)
       (local.tee $7
        (i32.shl
         (local.get $1)
         (i32.const 2)
        )
       )
      )
     )
     (br_if $label$6
      (i32.eqz
       (local.get $1)
      )
     )
     (br_if $label$6
      (i32.ne
       (i32.and
        (local.get $1)
        (i32.const 1073741823)
       )
       (local.get $1)
      )
     )
     (br_if $label$6
      (i32.lt_u
       (i32.add
        (local.get $7)
        (i32.const -2147483645)
       )
       (i32.const -2147483644)
      )
     )
     (call $__rust_dealloc
      (local.get $0)
      (local.get $7)
      (i32.const 4)
     )
    )
    (br_if $label$3
     (i32.ge_u
      (local.get $2)
      (local.get $5)
     )
    )
    (i32.store
     (i32.add
      (local.get $6)
      (i32.shl
       (local.get $2)
       (i32.const 2)
      )
     )
     (local.tee $2
      (i32.add
       (local.get $2)
       (i32.const 1)
      )
     )
    )
   )
   (br_if $label$3
    (i32.ge_u
     (local.get $4)
     (local.get $2)
    )
   )
   (br_if $label$3
    (i32.eqz
     (local.get $6)
    )
   )
   (local.set $1
    (i32.load
     (i32.add
      (local.get $6)
      (i32.shl
       (local.get $4)
       (i32.const 2)
      )
     )
    )
   )
   (i32.store offset=1048620
    (i32.const 0)
    (local.get $3)
   )
   (i32.store offset=1048616
    (i32.const 0)
    (local.get $1)
   )
   (i32.store offset=1048612
    (i32.const 0)
    (local.get $2)
   )
   (local.set $2
    (i32.load offset=1048608
     (i32.const 0)
    )
   )
   (i32.store offset=1048608
    (i32.const 0)
    (local.get $5)
   )
   (local.set $1
    (i32.load offset=1048604
     (i32.const 0)
    )
   )
   (i32.store offset=1048604
    (i32.const 0)
    (local.get $6)
   )
   (block $label$10
    (br_if $label$10
     (i32.eqz
      (local.get $2)
     )
    )
    (br_if $label$10
     (i32.ne
      (i32.and
       (local.get $2)
       (i32.const 1073741823)
      )
      (local.get $2)
     )
    )
    (br_if $label$10
     (i32.lt_u
      (i32.add
       (local.tee $6
        (i32.shl
         (local.get $2)
         (i32.const 2)
        )
       )
       (i32.const -2147483645)
      )
      (i32.const -2147483644)
     )
    )
    (call $__rust_dealloc
     (local.get $1)
     (local.get $6)
     (i32.const 4)
    )
   )
   (return
    (i32.add
     (local.get $3)
     (local.get $4)
    )
   )
  )
  (call $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE)
  (unreachable)
 )
 (func $__externref_table_dealloc (param $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.lt_u
      (local.get $0)
      (i32.const 36)
     )
    )
    (call $_ZN12wasm_bindgen9externref35__wbindgen_externref_table_set_null17h958e92ab89f726f8E
     (local.get $0)
    )
    (block $label$3
     (block $label$4
      (br_if $label$4
       (i32.eqz
        (i32.load offset=1048600
         (i32.const 0)
        )
       )
      )
      (local.set $1
       (i32.load offset=1048604
        (i32.const 0)
       )
      )
      (br $label$3)
     )
     (i64.store offset=1048616 align=4
      (i32.const 0)
      (i64.const 0)
     )
     (i64.store offset=1048608 align=4
      (i32.const 0)
      (i64.const 0)
     )
     (i32.store offset=1048600
      (i32.const 0)
      (i32.const 1)
     )
     (local.set $1
      (i32.const 4)
     )
    )
    (i32.store offset=1048604
     (i32.const 0)
     (i32.const 4)
    )
    (local.set $2
     (i32.load offset=1048612
      (i32.const 0)
     )
    )
    (local.set $3
     (i32.load offset=1048608
      (i32.const 0)
     )
    )
    (i64.store offset=1048608 align=4
     (i32.const 0)
     (i64.const 0)
    )
    (local.set $4
     (i32.load offset=1048616
      (i32.const 0)
     )
    )
    (local.set $5
     (i32.load offset=1048620
      (i32.const 0)
     )
    )
    (i64.store offset=1048616 align=4
     (i32.const 0)
     (i64.const 0)
    )
    (br_if $label$1
     (i32.gt_u
      (local.get $5)
      (local.get $0)
     )
    )
    (br_if $label$1
     (i32.ge_u
      (local.tee $0
       (i32.sub
        (local.get $0)
        (local.get $5)
       )
      )
      (local.get $2)
     )
    )
    (br_if $label$1
     (i32.eqz
      (local.get $1)
     )
    )
    (i32.store
     (i32.add
      (local.get $1)
      (i32.shl
       (local.get $0)
       (i32.const 2)
      )
     )
     (local.get $4)
    )
    (i32.store offset=1048620
     (i32.const 0)
     (local.get $5)
    )
    (i32.store offset=1048616
     (i32.const 0)
     (local.get $0)
    )
    (i32.store offset=1048612
     (i32.const 0)
     (local.get $2)
    )
    (local.set $0
     (i32.load offset=1048608
      (i32.const 0)
     )
    )
    (i32.store offset=1048608
     (i32.const 0)
     (local.get $3)
    )
    (local.set $5
     (i32.load offset=1048604
      (i32.const 0)
     )
    )
    (i32.store offset=1048604
     (i32.const 0)
     (local.get $1)
    )
    (br_if $label$2
     (i32.eqz
      (local.get $0)
     )
    )
    (br_if $label$2
     (i32.ne
      (i32.and
       (local.get $0)
       (i32.const 1073741823)
      )
      (local.get $0)
     )
    )
    (br_if $label$2
     (i32.lt_u
      (i32.add
       (local.tee $0
        (i32.shl
         (local.get $0)
         (i32.const 2)
        )
       )
       (i32.const -2147483645)
      )
      (i32.const -2147483644)
     )
    )
    (call $__rust_dealloc
     (local.get $5)
     (local.get $0)
     (i32.const 4)
    )
   )
   (return)
  )
  (call $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE)
  (unreachable)
 )
 (func $__externref_drop_slice (param $0 i32) (param $1 i32)
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $1)
    )
   )
   (local.set $1
    (i32.shl
     (local.get $1)
     (i32.const 2)
    )
   )
   (loop $label$2
    (call $__externref_table_dealloc
     (i32.load
      (local.get $0)
     )
    )
    (local.set $0
     (i32.add
      (local.get $0)
      (i32.const 4)
     )
    )
    (br_if $label$2
     (local.tee $1
      (i32.add
       (local.get $1)
       (i32.const -4)
      )
     )
    )
   )
  )
 )
 (func $__externref_heap_live_count (result i32)
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.eqz
      (i32.load offset=1048600
       (i32.const 0)
      )
     )
    )
    (local.set $0
     (i32.load offset=1048604
      (i32.const 0)
     )
    )
    (br $label$1)
   )
   (i64.store offset=1048616 align=4
    (i32.const 0)
    (i64.const 0)
   )
   (i64.store offset=1048608 align=4
    (i32.const 0)
    (i64.const 0)
   )
   (i32.store offset=1048600
    (i32.const 0)
    (i32.const 1)
   )
   (local.set $0
    (i32.const 4)
   )
  )
  (i32.store offset=1048604
   (i32.const 0)
   (i32.const 4)
  )
  (local.set $1
   (i32.load offset=1048608
    (i32.const 0)
   )
  )
  (local.set $2
   (i32.load offset=1048612
    (i32.const 0)
   )
  )
  (i64.store offset=1048608 align=4
   (i32.const 0)
   (i64.const 0)
  )
  (local.set $3
   (i32.load offset=1048620
    (i32.const 0)
   )
  )
  (local.set $4
   (i32.load offset=1048616
    (i32.const 0)
   )
  )
  (i64.store offset=1048616 align=4
   (i32.const 0)
   (i64.const 0)
  )
  (local.set $5
   (i32.const 0)
  )
  (block $label$3
   (block $label$4
    (br_if $label$4
     (i32.ge_u
      (local.get $4)
      (local.get $2)
     )
    )
    (br_if $label$3
     (i32.eqz
      (local.get $0)
     )
    )
    (local.set $5
     (i32.const 0)
    )
    (local.set $6
     (local.get $4)
    )
    (loop $label$5
     (br_if $label$3
      (i32.ge_u
       (local.get $6)
       (local.get $2)
      )
     )
     (local.set $5
      (i32.add
       (local.get $5)
       (i32.const 1)
      )
     )
     (br_if $label$5
      (i32.lt_u
       (local.tee $6
        (i32.load
         (i32.add
          (local.get $0)
          (i32.shl
           (local.get $6)
           (i32.const 2)
          )
         )
        )
       )
       (local.get $2)
      )
     )
    )
   )
   (i32.store offset=1048620
    (i32.const 0)
    (local.get $3)
   )
   (i32.store offset=1048616
    (i32.const 0)
    (local.get $4)
   )
   (i32.store offset=1048612
    (i32.const 0)
    (local.get $2)
   )
   (i32.store offset=1048604
    (i32.const 0)
    (local.get $0)
   )
   (local.set $6
    (i32.load offset=1048608
     (i32.const 0)
    )
   )
   (i32.store offset=1048608
    (i32.const 0)
    (local.get $1)
   )
   (block $label$6
    (br_if $label$6
     (i32.eqz
      (local.get $6)
     )
    )
    (br_if $label$6
     (i32.ne
      (i32.and
       (local.get $6)
       (i32.const 1073741823)
      )
      (local.get $6)
     )
    )
    (br_if $label$6
     (i32.lt_u
      (i32.add
       (local.tee $6
        (i32.shl
         (local.get $6)
         (i32.const 2)
        )
       )
       (i32.const -2147483645)
      )
      (i32.const -2147483644)
     )
    )
    (call $__rust_dealloc
     (i32.const 4)
     (local.get $6)
     (i32.const 4)
    )
   )
   (return
    (i32.sub
     (local.get $2)
     (local.get $5)
    )
   )
  )
  (call $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE)
  (unreachable)
 )
 (func $_ZN12wasm_bindgen9externref15link_intrinsics17h746d56ace27b8dbdE
 )
 (func $_ZN8dlmalloc17Dlmalloc$LT$A$GT$6malloc17he4572c35964f8c9bE (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (block $label$1
   (block $label$2
    (block $label$3
     (block $label$4
      (block $label$5
       (br_if $label$5
        (i32.lt_u
         (local.get $1)
         (i32.const 9)
        )
       )
       (br_if $label$4
        (i32.gt_u
         (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          (i32.const 16)
          (i32.const 8)
         )
         (local.get $1)
        )
       )
       (br $label$3)
      )
      (local.set $2
       (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E
        (local.get $0)
       )
      )
      (br $label$2)
     )
     (local.set $1
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (i32.const 16)
       (i32.const 8)
      )
     )
    )
    (local.set $3
     (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
    )
    (local.set $2
     (i32.const 0)
    )
    (br_if $label$2
     (i32.le_u
      (i32.sub
       (select
        (local.tee $3
         (i32.add
          (i32.and
           (i32.add
            (i32.sub
             (local.get $3)
             (i32.add
              (i32.add
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (local.get $3)
                (i32.const 8)
               )
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (i32.const 20)
                (i32.const 8)
               )
              )
              (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
               (i32.const 16)
               (i32.const 8)
              )
             )
            )
            (i32.const -65544)
           )
           (i32.const -9)
          )
          (i32.const -3)
         )
        )
        (local.tee $4
         (i32.sub
          (i32.const 0)
          (i32.shl
           (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            (i32.const 16)
            (i32.const 8)
           )
           (i32.const 2)
          )
         )
        )
        (i32.gt_u
         (local.get $4)
         (local.get $3)
        )
       )
       (local.get $1)
      )
      (local.get $0)
     )
    )
    (br_if $label$2
     (i32.eqz
      (local.tee $3
       (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E
        (i32.add
         (i32.add
          (i32.add
           (local.get $1)
           (local.tee $4
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (select
              (i32.const 16)
              (i32.add
               (local.get $0)
               (i32.const 4)
              )
              (i32.gt_u
               (i32.add
                (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                 (i32.const 16)
                 (i32.const 8)
                )
                (i32.const -5)
               )
               (local.get $0)
              )
             )
             (i32.const 8)
            )
           )
          )
          (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
           (i32.const 16)
           (i32.const 8)
          )
         )
         (i32.const -4)
        )
       )
      )
     )
    )
    (local.set $0
     (call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
      (local.get $3)
     )
    )
    (block $label$6
     (block $label$7
      (br_if $label$7
       (i32.and
        (local.tee $2
         (i32.add
          (local.get $1)
          (i32.const -1)
         )
        )
        (local.get $3)
       )
      )
      (local.set $1
       (local.get $0)
      )
      (br $label$6)
     )
     (local.set $2
      (call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
       (i32.and
        (i32.add
         (local.get $2)
         (local.get $3)
        )
        (i32.sub
         (i32.const 0)
         (local.get $1)
        )
       )
      )
     )
     (local.set $3
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (i32.const 16)
       (i32.const 8)
      )
     )
     (local.set $3
      (i32.sub
       (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
        (local.get $0)
       )
       (local.tee $2
        (i32.sub
         (local.tee $1
          (i32.add
           (local.get $2)
           (select
            (i32.const 0)
            (local.get $1)
            (i32.gt_u
             (i32.sub
              (local.get $2)
              (local.get $0)
             )
             (local.get $3)
            )
           )
          )
         )
         (local.get $0)
        )
       )
      )
     )
     (block $label$8
      (br_if $label$8
       (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
        (local.get $0)
       )
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
       (local.get $1)
       (local.get $3)
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
       (local.get $0)
       (local.get $2)
      )
      (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
       (local.get $0)
       (local.get $2)
      )
      (br $label$6)
     )
     (local.set $0
      (i32.load
       (local.get $0)
      )
     )
     (i32.store offset=4
      (local.get $1)
      (local.get $3)
     )
     (i32.store
      (local.get $1)
      (i32.add
       (local.get $0)
       (local.get $2)
      )
     )
    )
    (br_if $label$1
     (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
      (local.get $1)
     )
    )
    (br_if $label$1
     (i32.le_u
      (local.tee $0
       (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
        (local.get $1)
       )
      )
      (i32.add
       (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        (i32.const 16)
        (i32.const 8)
       )
       (local.get $4)
      )
     )
    )
    (local.set $2
     (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
      (local.get $1)
      (local.get $4)
     )
    )
    (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
     (local.get $1)
     (local.get $4)
    )
    (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
     (local.get $2)
     (local.tee $0
      (i32.sub
       (local.get $0)
       (local.get $4)
      )
     )
    )
    (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
     (local.get $2)
     (local.get $0)
    )
    (br $label$1)
   )
   (return
    (local.get $2)
   )
  )
  (local.set $0
   (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
    (local.get $1)
   )
  )
  (drop
   (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
    (local.get $1)
   )
  )
  (local.get $0)
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 i32)
  (local $11 i32)
  (local $12 i32)
  (local $13 i32)
  (local $14 i32)
  (local $15 i32)
  (local $16 i32)
  (local $17 i64)
  (global.set $__stack_pointer
   (local.tee $1
    (i32.sub
     (global.get $__stack_pointer)
     (i32.const 16)
    )
   )
  )
  (block $label$1
   (block $label$2
    (block $label$3
     (block $label$4
      (block $label$5
       (block $label$6
        (block $label$7
         (br_if $label$7
          (i32.lt_u
           (local.get $0)
           (i32.const 245)
          )
         )
         (local.set $2
          (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
         )
         (local.set $3
          (i32.const 0)
         )
         (br_if $label$1
          (i32.le_u
           (select
            (local.tee $2
             (i32.add
              (i32.and
               (i32.add
                (i32.sub
                 (local.get $2)
                 (i32.add
                  (i32.add
                   (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                    (local.get $2)
                    (i32.const 8)
                   )
                   (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                    (i32.const 20)
                    (i32.const 8)
                   )
                  )
                  (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                   (i32.const 16)
                   (i32.const 8)
                  )
                 )
                )
                (i32.const -65544)
               )
               (i32.const -9)
              )
              (i32.const -3)
             )
            )
            (local.tee $4
             (i32.sub
              (i32.const 0)
              (i32.shl
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (i32.const 16)
                (i32.const 8)
               )
               (i32.const 2)
              )
             )
            )
            (i32.gt_u
             (local.get $4)
             (local.get $2)
            )
           )
           (local.get $0)
          )
         )
         (local.set $2
          (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
           (i32.add
            (local.get $0)
            (i32.const 4)
           )
           (i32.const 8)
          )
         )
         (br_if $label$2
          (i32.eqz
           (i32.load offset=1048628
            (i32.const 0)
           )
          )
         )
         (local.set $5
          (i32.const 0)
         )
         (block $label$8
          (br_if $label$8
           (i32.lt_u
            (local.get $2)
            (i32.const 256)
           )
          )
          (local.set $5
           (i32.const 31)
          )
          (br_if $label$8
           (i32.gt_u
            (local.get $2)
            (i32.const 16777215)
           )
          )
          (local.set $5
           (i32.add
            (i32.sub
             (i32.and
              (i32.shr_u
               (local.get $2)
               (i32.sub
                (i32.const 6)
                (local.tee $0
                 (i32.clz
                  (i32.shr_u
                   (local.get $2)
                   (i32.const 8)
                  )
                 )
                )
               )
              )
              (i32.const 1)
             )
             (i32.shl
              (local.get $0)
              (i32.const 1)
             )
            )
            (i32.const 62)
           )
          )
         )
         (local.set $3
          (i32.sub
           (i32.const 0)
           (local.get $2)
          )
         )
         (br_if $label$6
          (local.tee $4
           (i32.load
            (i32.add
             (i32.shl
              (local.get $5)
              (i32.const 2)
             )
             (i32.const 1048896)
            )
           )
          )
         )
         (local.set $0
          (i32.const 0)
         )
         (local.set $6
          (i32.const 0)
         )
         (br $label$5)
        )
        (local.set $2
         (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          (select
           (i32.const 16)
           (i32.add
            (local.get $0)
            (i32.const 4)
           )
           (i32.gt_u
            (i32.add
             (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
              (i32.const 16)
              (i32.const 8)
             )
             (i32.const -5)
            )
            (local.get $0)
           )
          )
          (i32.const 8)
         )
        )
        (block $label$9
         (block $label$10
          (block $label$11
           (block $label$12
            (block $label$13
             (block $label$14
              (block $label$15
               (br_if $label$15
                (i32.and
                 (local.tee $0
                  (i32.shr_u
                   (local.tee $6
                    (i32.load offset=1048624
                     (i32.const 0)
                    )
                   )
                   (local.tee $3
                    (i32.shr_u
                     (local.get $2)
                     (i32.const 3)
                    )
                   )
                  )
                 )
                 (i32.const 3)
                )
               )
               (br_if $label$2
                (i32.le_u
                 (local.get $2)
                 (i32.load offset=1049024
                  (i32.const 0)
                 )
                )
               )
               (br_if $label$14
                (local.get $0)
               )
               (br_if $label$2
                (i32.eqz
                 (local.tee $0
                  (i32.load offset=1048628
                   (i32.const 0)
                  )
                 )
                )
               )
               (local.set $3
                (i32.sub
                 (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                  (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                   (local.tee $4
                    (i32.load
                     (i32.add
                      (i32.shl
                       (i32.ctz
                        (call $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE
                         (local.get $0)
                        )
                       )
                       (i32.const 2)
                      )
                      (i32.const 1048896)
                     )
                    )
                   )
                  )
                 )
                 (local.get $2)
                )
               )
               (block $label$16
                (br_if $label$16
                 (i32.eqz
                  (local.tee $0
                   (call $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE
                    (local.get $4)
                   )
                  )
                 )
                )
                (loop $label$17
                 (local.set $3
                  (select
                   (local.tee $6
                    (i32.sub
                     (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                      (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                       (local.get $0)
                      )
                     )
                     (local.get $2)
                    )
                   )
                   (local.get $3)
                   (local.tee $6
                    (i32.lt_u
                     (local.get $6)
                     (local.get $3)
                    )
                   )
                  )
                 )
                 (local.set $4
                  (select
                   (local.get $0)
                   (local.get $4)
                   (local.get $6)
                  )
                 )
                 (br_if $label$17
                  (local.tee $0
                   (call $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE
                    (local.get $0)
                   )
                  )
                 )
                )
               )
               (local.set $6
                (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                 (local.tee $0
                  (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                   (local.get $4)
                  )
                 )
                 (local.get $2)
                )
               )
               (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
                (i32.const 1048624)
                (local.get $4)
               )
               (br_if $label$10
                (i32.lt_u
                 (local.get $3)
                 (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  (i32.const 16)
                  (i32.const 8)
                 )
                )
               )
               (local.set $6
                (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                 (local.get $6)
                )
               )
               (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                (local.get $0)
                (local.get $2)
               )
               (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
                (local.get $6)
                (local.get $3)
               )
               (br_if $label$11
                (i32.eqz
                 (local.tee $4
                  (i32.load offset=1049024
                   (i32.const 0)
                  )
                 )
                )
               )
               (local.set $8
                (i32.add
                 (i32.shl
                  (local.tee $7
                   (i32.shr_u
                    (local.get $4)
                    (i32.const 3)
                   )
                  )
                  (i32.const 3)
                 )
                 (i32.const 1048632)
                )
               )
               (local.set $4
                (i32.load offset=1049032
                 (i32.const 0)
                )
               )
               (br_if $label$13
                (i32.eqz
                 (i32.and
                  (local.tee $5
                   (i32.load offset=1048624
                    (i32.const 0)
                   )
                  )
                  (local.tee $7
                   (i32.shl
                    (i32.const 1)
                    (local.get $7)
                   )
                  )
                 )
                )
               )
               (local.set $7
                (i32.load offset=8
                 (local.get $8)
                )
               )
               (br $label$12)
              )
              (block $label$18
               (block $label$19
                (br_if $label$19
                 (i32.eq
                  (local.tee $3
                   (i32.load
                    (i32.add
                     (local.tee $0
                      (i32.load
                       (i32.add
                        (local.tee $4
                         (i32.shl
                          (local.tee $2
                           (i32.add
                            (i32.and
                             (i32.xor
                              (local.get $0)
                              (i32.const -1)
                             )
                             (i32.const 1)
                            )
                            (local.get $3)
                           )
                          )
                          (i32.const 3)
                         )
                        )
                        (i32.const 1048640)
                       )
                      )
                     )
                     (i32.const 8)
                    )
                   )
                  )
                  (local.tee $4
                   (i32.add
                    (local.get $4)
                    (i32.const 1048632)
                   )
                  )
                 )
                )
                (i32.store offset=12
                 (local.get $3)
                 (local.get $4)
                )
                (i32.store offset=8
                 (local.get $4)
                 (local.get $3)
                )
                (br $label$18)
               )
               (i32.store offset=1048624
                (i32.const 0)
                (i32.and
                 (local.get $6)
                 (i32.rotl
                  (i32.const -2)
                  (local.get $2)
                 )
                )
               )
              )
              (call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
               (local.get $0)
               (i32.shl
                (local.get $2)
                (i32.const 3)
               )
              )
              (local.set $3
               (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                (local.get $0)
               )
              )
              (br $label$1)
             )
             (block $label$20
              (block $label$21
               (br_if $label$21
                (i32.eq
                 (local.tee $4
                  (i32.load
                   (i32.add
                    (local.tee $0
                     (i32.load
                      (i32.add
                       (local.tee $6
                        (i32.shl
                         (local.tee $3
                          (i32.ctz
                           (call $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE
                            (i32.and
                             (call $_ZN8dlmalloc8dlmalloc9left_bits17hd43e75bebd2d32bdE
                              (i32.shl
                               (i32.const 1)
                               (local.tee $3
                                (i32.and
                                 (local.get $3)
                                 (i32.const 31)
                                )
                               )
                              )
                             )
                             (i32.shl
                              (local.get $0)
                              (local.get $3)
                             )
                            )
                           )
                          )
                         )
                         (i32.const 3)
                        )
                       )
                       (i32.const 1048640)
                      )
                     )
                    )
                    (i32.const 8)
                   )
                  )
                 )
                 (local.tee $6
                  (i32.add
                   (local.get $6)
                   (i32.const 1048632)
                  )
                 )
                )
               )
               (i32.store offset=12
                (local.get $4)
                (local.get $6)
               )
               (i32.store offset=8
                (local.get $6)
                (local.get $4)
               )
               (br $label$20)
              )
              (i32.store offset=1048624
               (i32.const 0)
               (i32.and
                (i32.load offset=1048624
                 (i32.const 0)
                )
                (i32.rotl
                 (i32.const -2)
                 (local.get $3)
                )
               )
              )
             )
             (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
              (local.get $0)
              (local.get $2)
             )
             (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
              (local.tee $4
               (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                (local.get $0)
                (local.get $2)
               )
              )
              (local.tee $6
               (i32.sub
                (i32.shl
                 (local.get $3)
                 (i32.const 3)
                )
                (local.get $2)
               )
              )
             )
             (block $label$22
              (br_if $label$22
               (i32.eqz
                (local.tee $2
                 (i32.load offset=1049024
                  (i32.const 0)
                 )
                )
               )
              )
              (local.set $3
               (i32.add
                (i32.shl
                 (local.tee $8
                  (i32.shr_u
                   (local.get $2)
                   (i32.const 3)
                  )
                 )
                 (i32.const 3)
                )
                (i32.const 1048632)
               )
              )
              (local.set $2
               (i32.load offset=1049032
                (i32.const 0)
               )
              )
              (block $label$23
               (block $label$24
                (br_if $label$24
                 (i32.eqz
                  (i32.and
                   (local.tee $7
                    (i32.load offset=1048624
                     (i32.const 0)
                    )
                   )
                   (local.tee $8
                    (i32.shl
                     (i32.const 1)
                     (local.get $8)
                    )
                   )
                  )
                 )
                )
                (local.set $8
                 (i32.load offset=8
                  (local.get $3)
                 )
                )
                (br $label$23)
               )
               (i32.store offset=1048624
                (i32.const 0)
                (i32.or
                 (local.get $7)
                 (local.get $8)
                )
               )
               (local.set $8
                (local.get $3)
               )
              )
              (i32.store offset=8
               (local.get $3)
               (local.get $2)
              )
              (i32.store offset=12
               (local.get $8)
               (local.get $2)
              )
              (i32.store offset=12
               (local.get $2)
               (local.get $3)
              )
              (i32.store offset=8
               (local.get $2)
               (local.get $8)
              )
             )
             (i32.store offset=1049032
              (i32.const 0)
              (local.get $4)
             )
             (i32.store offset=1049024
              (i32.const 0)
              (local.get $6)
             )
             (local.set $3
              (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
               (local.get $0)
              )
             )
             (br $label$1)
            )
            (i32.store offset=1048624
             (i32.const 0)
             (i32.or
              (local.get $5)
              (local.get $7)
             )
            )
            (local.set $7
             (local.get $8)
            )
           )
           (i32.store offset=8
            (local.get $8)
            (local.get $4)
           )
           (i32.store offset=12
            (local.get $7)
            (local.get $4)
           )
           (i32.store offset=12
            (local.get $4)
            (local.get $8)
           )
           (i32.store offset=8
            (local.get $4)
            (local.get $7)
           )
          )
          (i32.store offset=1049032
           (i32.const 0)
           (local.get $6)
          )
          (i32.store offset=1049024
           (i32.const 0)
           (local.get $3)
          )
          (br $label$9)
         )
         (call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
          (local.get $0)
          (i32.add
           (local.get $3)
           (local.get $2)
          )
         )
        )
        (br_if $label$1
         (local.tee $3
          (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
           (local.get $0)
          )
         )
        )
        (br $label$2)
       )
       (local.set $8
        (i32.shl
         (local.get $2)
         (call $_ZN8dlmalloc8dlmalloc24leftshift_for_tree_index17hd789c537cab28411E
          (local.get $5)
         )
        )
       )
       (local.set $0
        (i32.const 0)
       )
       (local.set $6
        (i32.const 0)
       )
       (loop $label$25
        (block $label$26
         (br_if $label$26
          (i32.lt_u
           (local.tee $7
            (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
             (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
              (local.get $4)
             )
            )
           )
           (local.get $2)
          )
         )
         (br_if $label$26
          (i32.ge_u
           (local.tee $7
            (i32.sub
             (local.get $7)
             (local.get $2)
            )
           )
           (local.get $3)
          )
         )
         (local.set $3
          (local.get $7)
         )
         (local.set $6
          (local.get $4)
         )
         (br_if $label$26
          (local.get $7)
         )
         (local.set $3
          (i32.const 0)
         )
         (local.set $6
          (local.get $4)
         )
         (local.set $0
          (local.get $4)
         )
         (br $label$4)
        )
        (local.set $0
         (select
          (select
           (local.tee $7
            (i32.load
             (i32.add
              (local.get $4)
              (i32.const 20)
             )
            )
           )
           (local.get $0)
           (i32.ne
            (local.get $7)
            (local.tee $4
             (i32.load
              (i32.add
               (i32.add
                (local.get $4)
                (i32.and
                 (i32.shr_u
                  (local.get $8)
                  (i32.const 29)
                 )
                 (i32.const 4)
                )
               )
               (i32.const 16)
              )
             )
            )
           )
          )
          (local.get $0)
          (local.get $7)
         )
        )
        (local.set $8
         (i32.shl
          (local.get $8)
          (i32.const 1)
         )
        )
        (br_if $label$25
         (local.get $4)
        )
       )
      )
      (block $label$27
       (br_if $label$27
        (i32.or
         (local.get $0)
         (local.get $6)
        )
       )
       (local.set $6
        (i32.const 0)
       )
       (br_if $label$2
        (i32.eqz
         (local.tee $0
          (i32.and
           (call $_ZN8dlmalloc8dlmalloc9left_bits17hd43e75bebd2d32bdE
            (i32.shl
             (i32.const 1)
             (local.get $5)
            )
           )
           (i32.load offset=1048628
            (i32.const 0)
           )
          )
         )
        )
       )
       (local.set $0
        (i32.load
         (i32.add
          (i32.shl
           (i32.ctz
            (call $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE
             (local.get $0)
            )
           )
           (i32.const 2)
          )
          (i32.const 1048896)
         )
        )
       )
      )
      (br_if $label$3
       (i32.eqz
        (local.get $0)
       )
      )
     )
     (loop $label$28
      (local.set $6
       (select
        (local.get $0)
        (local.get $6)
        (local.tee $8
         (i32.and
          (i32.ge_u
           (local.tee $4
            (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
             (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
              (local.get $0)
             )
            )
           )
           (local.get $2)
          )
          (i32.lt_u
           (local.tee $4
            (i32.sub
             (local.get $4)
             (local.get $2)
            )
           )
           (local.get $3)
          )
         )
        )
       )
      )
      (local.set $3
       (select
        (local.get $4)
        (local.get $3)
        (local.get $8)
       )
      )
      (br_if $label$28
       (local.tee $0
        (call $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE
         (local.get $0)
        )
       )
      )
     )
    )
    (br_if $label$2
     (i32.eqz
      (local.get $6)
     )
    )
    (block $label$29
     (br_if $label$29
      (i32.lt_u
       (local.tee $0
        (i32.load offset=1049024
         (i32.const 0)
        )
       )
       (local.get $2)
      )
     )
     (br_if $label$2
      (i32.ge_u
       (local.get $3)
       (i32.sub
        (local.get $0)
        (local.get $2)
       )
      )
     )
    )
    (local.set $4
     (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
      (local.tee $0
       (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
        (local.get $6)
       )
      )
      (local.get $2)
     )
    )
    (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
     (i32.const 1048624)
     (local.get $6)
    )
    (block $label$30
     (block $label$31
      (br_if $label$31
       (i32.lt_u
        (local.get $3)
        (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
         (i32.const 16)
         (i32.const 8)
        )
       )
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
       (local.get $0)
       (local.get $2)
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
       (local.get $4)
       (local.get $3)
      )
      (block $label$32
       (br_if $label$32
        (i32.lt_u
         (local.get $3)
         (i32.const 256)
        )
       )
       (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
        (i32.const 1048624)
        (local.get $4)
        (local.get $3)
       )
       (br $label$30)
      )
      (local.set $3
       (i32.add
        (i32.shl
         (local.tee $6
          (i32.shr_u
           (local.get $3)
           (i32.const 3)
          )
         )
         (i32.const 3)
        )
        (i32.const 1048632)
       )
      )
      (block $label$33
       (block $label$34
        (br_if $label$34
         (i32.eqz
          (i32.and
           (local.tee $8
            (i32.load offset=1048624
             (i32.const 0)
            )
           )
           (local.tee $6
            (i32.shl
             (i32.const 1)
             (local.get $6)
            )
           )
          )
         )
        )
        (local.set $6
         (i32.load offset=8
          (local.get $3)
         )
        )
        (br $label$33)
       )
       (i32.store offset=1048624
        (i32.const 0)
        (i32.or
         (local.get $8)
         (local.get $6)
        )
       )
       (local.set $6
        (local.get $3)
       )
      )
      (i32.store offset=8
       (local.get $3)
       (local.get $4)
      )
      (i32.store offset=12
       (local.get $6)
       (local.get $4)
      )
      (i32.store offset=12
       (local.get $4)
       (local.get $3)
      )
      (i32.store offset=8
       (local.get $4)
       (local.get $6)
      )
      (br $label$30)
     )
     (call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
      (local.get $0)
      (i32.add
       (local.get $3)
       (local.get $2)
      )
     )
    )
    (br_if $label$1
     (local.tee $3
      (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
       (local.get $0)
      )
     )
    )
   )
   (block $label$35
    (block $label$36
     (block $label$37
      (block $label$38
       (block $label$39
        (block $label$40
         (block $label$41
          (block $label$42
           (block $label$43
            (br_if $label$43
             (i32.ge_u
              (local.tee $3
               (i32.load offset=1049024
                (i32.const 0)
               )
              )
              (local.get $2)
             )
            )
            (br_if $label$41
             (i32.gt_u
              (local.tee $0
               (i32.load offset=1049028
                (i32.const 0)
               )
              )
              (local.get $2)
             )
            )
            (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5alloc17he1272ca423b0b1b4E
             (local.get $1)
             (i32.const 1048624)
             (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
              (i32.add
               (i32.add
                (i32.add
                 (i32.add
                  (i32.sub
                   (local.get $2)
                   (local.tee $0
                    (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
                   )
                  )
                  (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                   (local.get $0)
                   (i32.const 8)
                  )
                 )
                 (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  (i32.const 20)
                  (i32.const 8)
                 )
                )
                (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                 (i32.const 16)
                 (i32.const 8)
                )
               )
               (i32.const 8)
              )
              (i32.const 65536)
             )
            )
            (br_if $label$42
             (local.tee $3
              (i32.load
               (local.get $1)
              )
             )
            )
            (local.set $3
             (i32.const 0)
            )
            (br $label$1)
           )
           (local.set $0
            (i32.load offset=1049032
             (i32.const 0)
            )
           )
           (block $label$44
            (br_if $label$44
             (i32.ge_u
              (local.tee $3
               (i32.sub
                (local.get $3)
                (local.get $2)
               )
              )
              (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
               (i32.const 16)
               (i32.const 8)
              )
             )
            )
            (i32.store offset=1049032
             (i32.const 0)
             (i32.const 0)
            )
            (local.set $2
             (i32.load offset=1049024
              (i32.const 0)
             )
            )
            (i32.store offset=1049024
             (i32.const 0)
             (i32.const 0)
            )
            (call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
             (local.get $0)
             (local.get $2)
            )
            (local.set $3
             (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
              (local.get $0)
             )
            )
            (br $label$1)
           )
           (local.set $4
            (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
             (local.get $0)
             (local.get $2)
            )
           )
           (i32.store offset=1049024
            (i32.const 0)
            (local.get $3)
           )
           (i32.store offset=1049032
            (i32.const 0)
            (local.get $4)
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
            (local.get $4)
            (local.get $3)
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
            (local.get $0)
            (local.get $2)
           )
           (local.set $3
            (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
             (local.get $0)
            )
           )
           (br $label$1)
          )
          (local.set $5
           (i32.load offset=8
            (local.get $1)
           )
          )
          (i32.store offset=1049040
           (i32.const 0)
           (local.tee $0
            (i32.add
             (i32.load offset=1049040
              (i32.const 0)
             )
             (local.tee $8
              (i32.load offset=4
               (local.get $1)
              )
             )
            )
           )
          )
          (i32.store offset=1049044
           (i32.const 0)
           (select
            (local.tee $4
             (i32.load offset=1049044
              (i32.const 0)
             )
            )
            (local.get $0)
            (i32.gt_u
             (local.get $4)
             (local.get $0)
            )
           )
          )
          (block $label$45
           (block $label$46
            (block $label$47
             (br_if $label$47
              (i32.eqz
               (i32.load offset=1049036
                (i32.const 0)
               )
              )
             )
             (local.set $0
              (i32.const 1049048)
             )
             (loop $label$48
              (br_if $label$46
               (i32.eq
                (local.get $3)
                (call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                 (local.get $0)
                )
               )
              )
              (br_if $label$48
               (local.tee $0
                (i32.load offset=8
                 (local.get $0)
                )
               )
              )
              (br $label$45)
             )
            )
            (br_if $label$40
             (i32.eqz
              (local.tee $0
               (i32.load offset=1049068
                (i32.const 0)
               )
              )
             )
            )
            (br_if $label$40
             (i32.lt_u
              (local.get $3)
              (local.get $0)
             )
            )
            (br $label$36)
           )
           (br_if $label$45
            (call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
             (local.get $0)
            )
           )
           (br_if $label$45
            (i32.ne
             (call $_ZN8dlmalloc8dlmalloc7Segment9sys_flags17h6d168430a1d92f9aE
              (local.get $0)
             )
             (local.get $5)
            )
           )
           (br_if $label$39
            (call $_ZN8dlmalloc8dlmalloc7Segment5holds17h276a4b63e2947208E
             (local.get $0)
             (i32.load offset=1049036
              (i32.const 0)
             )
            )
           )
          )
          (i32.store offset=1049068
           (i32.const 0)
           (select
            (local.tee $0
             (i32.load offset=1049068
              (i32.const 0)
             )
            )
            (local.get $3)
            (i32.gt_u
             (local.get $3)
             (local.get $0)
            )
           )
          )
          (local.set $4
           (i32.add
            (local.get $3)
            (local.get $8)
           )
          )
          (local.set $0
           (i32.const 1049048)
          )
          (block $label$49
           (block $label$50
            (block $label$51
             (loop $label$52
              (br_if $label$51
               (i32.eq
                (i32.load
                 (local.get $0)
                )
                (local.get $4)
               )
              )
              (br_if $label$52
               (local.tee $0
                (i32.load offset=8
                 (local.get $0)
                )
               )
              )
              (br $label$50)
             )
            )
            (br_if $label$50
             (call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
              (local.get $0)
             )
            )
            (br_if $label$49
             (i32.eq
              (call $_ZN8dlmalloc8dlmalloc7Segment9sys_flags17h6d168430a1d92f9aE
               (local.get $0)
              )
              (local.get $5)
             )
            )
           )
           (local.set $4
            (i32.load offset=1049036
             (i32.const 0)
            )
           )
           (local.set $0
            (i32.const 1049048)
           )
           (block $label$53
            (loop $label$54
             (block $label$55
              (br_if $label$55
               (i32.gt_u
                (i32.load
                 (local.get $0)
                )
                (local.get $4)
               )
              )
              (br_if $label$53
               (i32.gt_u
                (call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                 (local.get $0)
                )
                (local.get $4)
               )
              )
             )
             (br_if $label$54
              (local.tee $0
               (i32.load offset=8
                (local.get $0)
               )
              )
             )
            )
            (local.set $0
             (i32.const 0)
            )
           )
           (local.set $0
            (i32.add
             (i32.sub
              (local.tee $6
               (call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                (local.get $0)
               )
              )
              (local.tee $9
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (i32.const 20)
                (i32.const 8)
               )
              )
             )
             (i32.const -23)
            )
           )
           (local.set $10
            (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
             (local.tee $7
              (select
               (local.get $4)
               (local.tee $0
                (i32.add
                 (local.get $0)
                 (i32.sub
                  (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                   (local.tee $7
                    (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                     (local.get $0)
                    )
                   )
                   (i32.const 8)
                  )
                  (local.get $7)
                 )
                )
               )
               (i32.lt_u
                (local.get $0)
                (i32.add
                 (local.get $4)
                 (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  (i32.const 16)
                  (i32.const 8)
                 )
                )
               )
              )
             )
            )
           )
           (local.set $0
            (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
             (local.get $7)
             (local.get $9)
            )
           )
           (local.set $12
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (local.tee $11
              (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
             )
             (i32.const 8)
            )
           )
           (local.set $13
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (i32.const 20)
             (i32.const 8)
            )
           )
           (local.set $14
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (i32.const 16)
             (i32.const 8)
            )
           )
           (i32.store offset=1049036
            (i32.const 0)
            (local.tee $15
             (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
              (local.get $3)
              (local.tee $16
               (i32.sub
                (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                 (local.tee $15
                  (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                   (local.get $3)
                  )
                 )
                 (i32.const 8)
                )
                (local.get $15)
               )
              )
             )
            )
           )
           (i32.store offset=1049028
            (i32.const 0)
            (local.tee $11
             (i32.sub
              (i32.add
               (local.get $11)
               (local.get $8)
              )
              (i32.add
               (i32.add
                (local.get $14)
                (i32.add
                 (local.get $12)
                 (local.get $13)
                )
               )
               (local.get $16)
              )
             )
            )
           )
           (i32.store offset=4
            (local.get $15)
            (i32.or
             (local.get $11)
             (i32.const 1)
            )
           )
           (local.set $13
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (local.tee $12
              (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
             )
             (i32.const 8)
            )
           )
           (local.set $14
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (i32.const 20)
             (i32.const 8)
            )
           )
           (local.set $16
            (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
             (i32.const 16)
             (i32.const 8)
            )
           )
           (local.set $15
            (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
             (local.get $15)
             (local.get $11)
            )
           )
           (i32.store offset=1049064
            (i32.const 0)
            (i32.const 2097152)
           )
           (i32.store offset=4
            (local.get $15)
            (i32.add
             (local.get $16)
             (i32.add
              (local.get $14)
              (i32.sub
               (local.get $13)
               (local.get $12)
              )
             )
            )
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
            (local.get $7)
            (local.get $9)
           )
           (local.set $17
            (i64.load offset=1049048 align=4
             (i32.const 0)
            )
           )
           (i64.store align=4
            (i32.add
             (local.get $10)
             (i32.const 8)
            )
            (i64.load offset=1049056 align=4
             (i32.const 0)
            )
           )
           (i64.store align=4
            (local.get $10)
            (local.get $17)
           )
           (i32.store offset=1049060
            (i32.const 0)
            (local.get $5)
           )
           (i32.store offset=1049052
            (i32.const 0)
            (local.get $8)
           )
           (i32.store offset=1049048
            (i32.const 0)
            (local.get $3)
           )
           (i32.store offset=1049056
            (i32.const 0)
            (local.get $10)
           )
           (loop $label$56
            (local.set $3
             (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
              (local.get $0)
              (i32.const 4)
             )
            )
            (i32.store offset=4
             (local.get $0)
             (call $_ZN8dlmalloc8dlmalloc5Chunk14fencepost_head17h32cfaa035be31489E)
            )
            (local.set $0
             (local.get $3)
            )
            (br_if $label$56
             (i32.gt_u
              (local.get $6)
              (i32.add
               (local.get $3)
               (i32.const 4)
              )
             )
            )
           )
           (br_if $label$35
            (i32.eq
             (local.get $7)
             (local.get $4)
            )
           )
           (local.set $0
            (i32.sub
             (local.get $7)
             (local.get $4)
            )
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
            (local.get $4)
            (local.get $0)
            (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
             (local.get $4)
             (local.get $0)
            )
           )
           (block $label$57
            (br_if $label$57
             (i32.lt_u
              (local.get $0)
              (i32.const 256)
             )
            )
            (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
             (i32.const 1048624)
             (local.get $4)
             (local.get $0)
            )
            (br $label$35)
           )
           (local.set $0
            (i32.add
             (i32.shl
              (local.tee $3
               (i32.shr_u
                (local.get $0)
                (i32.const 3)
               )
              )
              (i32.const 3)
             )
             (i32.const 1048632)
            )
           )
           (block $label$58
            (block $label$59
             (br_if $label$59
              (i32.eqz
               (i32.and
                (local.tee $6
                 (i32.load offset=1048624
                  (i32.const 0)
                 )
                )
                (local.tee $3
                 (i32.shl
                  (i32.const 1)
                  (local.get $3)
                 )
                )
               )
              )
             )
             (local.set $3
              (i32.load offset=8
               (local.get $0)
              )
             )
             (br $label$58)
            )
            (i32.store offset=1048624
             (i32.const 0)
             (i32.or
              (local.get $6)
              (local.get $3)
             )
            )
            (local.set $3
             (local.get $0)
            )
           )
           (i32.store offset=8
            (local.get $0)
            (local.get $4)
           )
           (i32.store offset=12
            (local.get $3)
            (local.get $4)
           )
           (i32.store offset=12
            (local.get $4)
            (local.get $0)
           )
           (i32.store offset=8
            (local.get $4)
            (local.get $3)
           )
           (br $label$35)
          )
          (local.set $6
           (i32.load
            (local.get $0)
           )
          )
          (i32.store
           (local.get $0)
           (local.get $3)
          )
          (i32.store offset=4
           (local.get $0)
           (i32.add
            (i32.load offset=4
             (local.get $0)
            )
            (local.get $8)
           )
          )
          (local.set $4
           (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            (local.tee $0
             (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
              (local.get $3)
             )
            )
            (i32.const 8)
           )
          )
          (local.set $7
           (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            (local.tee $8
             (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
              (local.get $6)
             )
            )
            (i32.const 8)
           )
          )
          (local.set $4
           (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
            (local.tee $3
             (i32.add
              (local.get $3)
              (i32.sub
               (local.get $4)
               (local.get $0)
              )
             )
            )
            (local.get $2)
           )
          )
          (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
           (local.get $3)
           (local.get $2)
          )
          (local.set $2
           (i32.sub
            (local.tee $0
             (i32.add
              (local.get $6)
              (i32.sub
               (local.get $7)
               (local.get $8)
              )
             )
            )
            (i32.add
             (local.get $2)
             (local.get $3)
            )
           )
          )
          (block $label$60
           (br_if $label$60
            (i32.eq
             (i32.load offset=1049036
              (i32.const 0)
             )
             (local.get $0)
            )
           )
           (br_if $label$38
            (i32.eq
             (i32.load offset=1049032
              (i32.const 0)
             )
             (local.get $0)
            )
           )
           (br_if $label$37
            (call $_ZN8dlmalloc8dlmalloc5Chunk5inuse17h4d9d8a6e39f8aee5E
             (local.get $0)
            )
           )
           (block $label$61
            (block $label$62
             (br_if $label$62
              (i32.lt_u
               (local.tee $6
                (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                 (local.get $0)
                )
               )
               (i32.const 256)
              )
             )
             (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
              (i32.const 1048624)
              (local.get $0)
             )
             (br $label$61)
            )
            (block $label$63
             (br_if $label$63
              (i32.eq
               (local.tee $8
                (i32.load
                 (i32.add
                  (local.get $0)
                  (i32.const 12)
                 )
                )
               )
               (local.tee $7
                (i32.load
                 (i32.add
                  (local.get $0)
                  (i32.const 8)
                 )
                )
               )
              )
             )
             (i32.store offset=12
              (local.get $7)
              (local.get $8)
             )
             (i32.store offset=8
              (local.get $8)
              (local.get $7)
             )
             (br $label$61)
            )
            (i32.store offset=1048624
             (i32.const 0)
             (i32.and
              (i32.load offset=1048624
               (i32.const 0)
              )
              (i32.rotl
               (i32.const -2)
               (i32.shr_u
                (local.get $6)
                (i32.const 3)
               )
              )
             )
            )
           )
           (local.set $2
            (i32.add
             (local.get $6)
             (local.get $2)
            )
           )
           (local.set $0
            (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
             (local.get $0)
             (local.get $6)
            )
           )
           (br $label$37)
          )
          (i32.store offset=1049036
           (i32.const 0)
           (local.get $4)
          )
          (i32.store offset=1049028
           (i32.const 0)
           (local.tee $0
            (i32.add
             (i32.load offset=1049028
              (i32.const 0)
             )
             (local.get $2)
            )
           )
          )
          (i32.store offset=4
           (local.get $4)
           (i32.or
            (local.get $0)
            (i32.const 1)
           )
          )
          (local.set $3
           (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
            (local.get $3)
           )
          )
          (br $label$1)
         )
         (i32.store offset=1049028
          (i32.const 0)
          (local.tee $3
           (i32.sub
            (local.get $0)
            (local.get $2)
           )
          )
         )
         (i32.store offset=1049036
          (i32.const 0)
          (local.tee $4
           (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
            (local.tee $0
             (i32.load offset=1049036
              (i32.const 0)
             )
            )
            (local.get $2)
           )
          )
         )
         (i32.store offset=4
          (local.get $4)
          (i32.or
           (local.get $3)
           (i32.const 1)
          )
         )
         (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
          (local.get $0)
          (local.get $2)
         )
         (local.set $3
          (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
           (local.get $0)
          )
         )
         (br $label$1)
        )
        (i32.store offset=1049068
         (i32.const 0)
         (local.get $3)
        )
        (br $label$36)
       )
       (i32.store offset=4
        (local.get $0)
        (i32.add
         (i32.load offset=4
          (local.get $0)
         )
         (local.get $8)
        )
       )
       (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$8init_top17h9ba4d179485fee16E
        (i32.const 1048624)
        (i32.load offset=1049036
         (i32.const 0)
        )
        (i32.add
         (i32.load offset=1049028
          (i32.const 0)
         )
         (local.get $8)
        )
       )
       (br $label$35)
      )
      (i32.store offset=1049032
       (i32.const 0)
       (local.get $4)
      )
      (i32.store offset=1049024
       (i32.const 0)
       (local.tee $0
        (i32.add
         (i32.load offset=1049024
          (i32.const 0)
         )
         (local.get $2)
        )
       )
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
       (local.get $4)
       (local.get $0)
      )
      (local.set $3
       (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
        (local.get $3)
       )
      )
      (br $label$1)
     )
     (call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
      (local.get $4)
      (local.get $2)
      (local.get $0)
     )
     (block $label$64
      (br_if $label$64
       (i32.lt_u
        (local.get $2)
        (i32.const 256)
       )
      )
      (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
       (i32.const 1048624)
       (local.get $4)
       (local.get $2)
      )
      (local.set $3
       (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
        (local.get $3)
       )
      )
      (br $label$1)
     )
     (local.set $0
      (i32.add
       (i32.shl
        (local.tee $2
         (i32.shr_u
          (local.get $2)
          (i32.const 3)
         )
        )
        (i32.const 3)
       )
       (i32.const 1048632)
      )
     )
     (block $label$65
      (block $label$66
       (br_if $label$66
        (i32.eqz
         (i32.and
          (local.tee $6
           (i32.load offset=1048624
            (i32.const 0)
           )
          )
          (local.tee $2
           (i32.shl
            (i32.const 1)
            (local.get $2)
           )
          )
         )
        )
       )
       (local.set $2
        (i32.load offset=8
         (local.get $0)
        )
       )
       (br $label$65)
      )
      (i32.store offset=1048624
       (i32.const 0)
       (i32.or
        (local.get $6)
        (local.get $2)
       )
      )
      (local.set $2
       (local.get $0)
      )
     )
     (i32.store offset=8
      (local.get $0)
      (local.get $4)
     )
     (i32.store offset=12
      (local.get $2)
      (local.get $4)
     )
     (i32.store offset=12
      (local.get $4)
      (local.get $0)
     )
     (i32.store offset=8
      (local.get $4)
      (local.get $2)
     )
     (local.set $3
      (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
       (local.get $3)
      )
     )
     (br $label$1)
    )
    (i32.store offset=1049072
     (i32.const 0)
     (i32.const 4095)
    )
    (i32.store offset=1049060
     (i32.const 0)
     (local.get $5)
    )
    (i32.store offset=1049052
     (i32.const 0)
     (local.get $8)
    )
    (i32.store offset=1049048
     (i32.const 0)
     (local.get $3)
    )
    (i32.store offset=1048644
     (i32.const 0)
     (i32.const 1048632)
    )
    (i32.store offset=1048652
     (i32.const 0)
     (i32.const 1048640)
    )
    (i32.store offset=1048640
     (i32.const 0)
     (i32.const 1048632)
    )
    (i32.store offset=1048660
     (i32.const 0)
     (i32.const 1048648)
    )
    (i32.store offset=1048648
     (i32.const 0)
     (i32.const 1048640)
    )
    (i32.store offset=1048668
     (i32.const 0)
     (i32.const 1048656)
    )
    (i32.store offset=1048656
     (i32.const 0)
     (i32.const 1048648)
    )
    (i32.store offset=1048676
     (i32.const 0)
     (i32.const 1048664)
    )
    (i32.store offset=1048664
     (i32.const 0)
     (i32.const 1048656)
    )
    (i32.store offset=1048684
     (i32.const 0)
     (i32.const 1048672)
    )
    (i32.store offset=1048672
     (i32.const 0)
     (i32.const 1048664)
    )
    (i32.store offset=1048692
     (i32.const 0)
     (i32.const 1048680)
    )
    (i32.store offset=1048680
     (i32.const 0)
     (i32.const 1048672)
    )
    (i32.store offset=1048700
     (i32.const 0)
     (i32.const 1048688)
    )
    (i32.store offset=1048688
     (i32.const 0)
     (i32.const 1048680)
    )
    (i32.store offset=1048708
     (i32.const 0)
     (i32.const 1048696)
    )
    (i32.store offset=1048696
     (i32.const 0)
     (i32.const 1048688)
    )
    (i32.store offset=1048704
     (i32.const 0)
     (i32.const 1048696)
    )
    (i32.store offset=1048716
     (i32.const 0)
     (i32.const 1048704)
    )
    (i32.store offset=1048712
     (i32.const 0)
     (i32.const 1048704)
    )
    (i32.store offset=1048724
     (i32.const 0)
     (i32.const 1048712)
    )
    (i32.store offset=1048720
     (i32.const 0)
     (i32.const 1048712)
    )
    (i32.store offset=1048732
     (i32.const 0)
     (i32.const 1048720)
    )
    (i32.store offset=1048728
     (i32.const 0)
     (i32.const 1048720)
    )
    (i32.store offset=1048740
     (i32.const 0)
     (i32.const 1048728)
    )
    (i32.store offset=1048736
     (i32.const 0)
     (i32.const 1048728)
    )
    (i32.store offset=1048748
     (i32.const 0)
     (i32.const 1048736)
    )
    (i32.store offset=1048744
     (i32.const 0)
     (i32.const 1048736)
    )
    (i32.store offset=1048756
     (i32.const 0)
     (i32.const 1048744)
    )
    (i32.store offset=1048752
     (i32.const 0)
     (i32.const 1048744)
    )
    (i32.store offset=1048764
     (i32.const 0)
     (i32.const 1048752)
    )
    (i32.store offset=1048760
     (i32.const 0)
     (i32.const 1048752)
    )
    (i32.store offset=1048772
     (i32.const 0)
     (i32.const 1048760)
    )
    (i32.store offset=1048780
     (i32.const 0)
     (i32.const 1048768)
    )
    (i32.store offset=1048768
     (i32.const 0)
     (i32.const 1048760)
    )
    (i32.store offset=1048788
     (i32.const 0)
     (i32.const 1048776)
    )
    (i32.store offset=1048776
     (i32.const 0)
     (i32.const 1048768)
    )
    (i32.store offset=1048796
     (i32.const 0)
     (i32.const 1048784)
    )
    (i32.store offset=1048784
     (i32.const 0)
     (i32.const 1048776)
    )
    (i32.store offset=1048804
     (i32.const 0)
     (i32.const 1048792)
    )
    (i32.store offset=1048792
     (i32.const 0)
     (i32.const 1048784)
    )
    (i32.store offset=1048812
     (i32.const 0)
     (i32.const 1048800)
    )
    (i32.store offset=1048800
     (i32.const 0)
     (i32.const 1048792)
    )
    (i32.store offset=1048820
     (i32.const 0)
     (i32.const 1048808)
    )
    (i32.store offset=1048808
     (i32.const 0)
     (i32.const 1048800)
    )
    (i32.store offset=1048828
     (i32.const 0)
     (i32.const 1048816)
    )
    (i32.store offset=1048816
     (i32.const 0)
     (i32.const 1048808)
    )
    (i32.store offset=1048836
     (i32.const 0)
     (i32.const 1048824)
    )
    (i32.store offset=1048824
     (i32.const 0)
     (i32.const 1048816)
    )
    (i32.store offset=1048844
     (i32.const 0)
     (i32.const 1048832)
    )
    (i32.store offset=1048832
     (i32.const 0)
     (i32.const 1048824)
    )
    (i32.store offset=1048852
     (i32.const 0)
     (i32.const 1048840)
    )
    (i32.store offset=1048840
     (i32.const 0)
     (i32.const 1048832)
    )
    (i32.store offset=1048860
     (i32.const 0)
     (i32.const 1048848)
    )
    (i32.store offset=1048848
     (i32.const 0)
     (i32.const 1048840)
    )
    (i32.store offset=1048868
     (i32.const 0)
     (i32.const 1048856)
    )
    (i32.store offset=1048856
     (i32.const 0)
     (i32.const 1048848)
    )
    (i32.store offset=1048876
     (i32.const 0)
     (i32.const 1048864)
    )
    (i32.store offset=1048864
     (i32.const 0)
     (i32.const 1048856)
    )
    (i32.store offset=1048884
     (i32.const 0)
     (i32.const 1048872)
    )
    (i32.store offset=1048872
     (i32.const 0)
     (i32.const 1048864)
    )
    (i32.store offset=1048892
     (i32.const 0)
     (i32.const 1048880)
    )
    (i32.store offset=1048880
     (i32.const 0)
     (i32.const 1048872)
    )
    (i32.store offset=1048888
     (i32.const 0)
     (i32.const 1048880)
    )
    (local.set $6
     (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
      (local.tee $4
       (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
      )
      (i32.const 8)
     )
    )
    (local.set $7
     (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
      (i32.const 20)
      (i32.const 8)
     )
    )
    (local.set $5
     (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
      (i32.const 16)
      (i32.const 8)
     )
    )
    (i32.store offset=1049036
     (i32.const 0)
     (local.tee $0
      (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
       (local.get $3)
       (local.tee $10
        (i32.sub
         (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          (local.tee $0
           (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
            (local.get $3)
           )
          )
          (i32.const 8)
         )
         (local.get $0)
        )
       )
      )
     )
    )
    (i32.store offset=1049028
     (i32.const 0)
     (local.tee $3
      (i32.sub
       (i32.add
        (local.get $4)
        (local.get $8)
       )
       (i32.add
        (i32.add
         (local.get $5)
         (i32.add
          (local.get $6)
          (local.get $7)
         )
        )
        (local.get $10)
       )
      )
     )
    )
    (i32.store offset=4
     (local.get $0)
     (i32.or
      (local.get $3)
      (i32.const 1)
     )
    )
    (local.set $6
     (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
      (local.tee $4
       (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
      )
      (i32.const 8)
     )
    )
    (local.set $8
     (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
      (i32.const 20)
      (i32.const 8)
     )
    )
    (local.set $7
     (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
      (i32.const 16)
      (i32.const 8)
     )
    )
    (local.set $0
     (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
      (local.get $0)
      (local.get $3)
     )
    )
    (i32.store offset=1049064
     (i32.const 0)
     (i32.const 2097152)
    )
    (i32.store offset=4
     (local.get $0)
     (i32.add
      (local.get $7)
      (i32.add
       (local.get $8)
       (i32.sub
        (local.get $6)
        (local.get $4)
       )
      )
     )
    )
   )
   (local.set $3
    (i32.const 0)
   )
   (br_if $label$1
    (i32.le_u
     (local.tee $0
      (i32.load offset=1049028
       (i32.const 0)
      )
     )
     (local.get $2)
    )
   )
   (i32.store offset=1049028
    (i32.const 0)
    (local.tee $3
     (i32.sub
      (local.get $0)
      (local.get $2)
     )
    )
   )
   (i32.store offset=1049036
    (i32.const 0)
    (local.tee $4
     (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
      (local.tee $0
       (i32.load offset=1049036
        (i32.const 0)
       )
      )
      (local.get $2)
     )
    )
   )
   (i32.store offset=4
    (local.get $4)
    (i32.or
     (local.get $3)
     (i32.const 1)
    )
   )
   (call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
    (local.get $0)
    (local.get $2)
   )
   (local.set $3
    (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
     (local.get $0)
    )
   )
  )
  (global.set $__stack_pointer
   (i32.add
    (local.get $1)
    (i32.const 16)
   )
  )
  (local.get $3)
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local.set $2
   (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    (local.get $0)
    (local.get $1)
   )
  )
  (block $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (call $_ZN8dlmalloc8dlmalloc5Chunk6pinuse17h89f5f80c1a4cb95aE
       (local.get $0)
      )
     )
     (local.set $3
      (i32.load
       (local.get $0)
      )
     )
     (block $label$4
      (block $label$5
       (br_if $label$5
        (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
         (local.get $0)
        )
       )
       (local.set $1
        (i32.add
         (local.get $3)
         (local.get $1)
        )
       )
       (br_if $label$4
        (i32.ne
         (local.tee $0
          (call $_ZN8dlmalloc8dlmalloc5Chunk12minus_offset17h39dd10694c91288eE
           (local.get $0)
           (local.get $3)
          )
         )
         (i32.load offset=1049032
          (i32.const 0)
         )
        )
       )
       (br_if $label$3
        (i32.ne
         (i32.and
          (i32.load offset=4
           (local.get $2)
          )
          (i32.const 3)
         )
         (i32.const 3)
        )
       )
       (i32.store offset=1049024
        (i32.const 0)
        (local.get $1)
       )
       (call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
        (local.get $0)
        (local.get $1)
        (local.get $2)
       )
       (return)
      )
      (br_if $label$2
       (i32.eqz
        (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E
         (i32.const 1048624)
         (i32.sub
          (local.get $0)
          (local.get $3)
         )
         (local.tee $0
          (i32.add
           (i32.add
            (local.get $3)
            (local.get $1)
           )
           (i32.const 16)
          )
         )
        )
       )
      )
      (i32.store offset=1049040
       (i32.const 0)
       (i32.sub
        (i32.load offset=1049040
         (i32.const 0)
        )
        (local.get $0)
       )
      )
      (return)
     )
     (block $label$6
      (br_if $label$6
       (i32.lt_u
        (local.get $3)
        (i32.const 256)
       )
      )
      (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
       (i32.const 1048624)
       (local.get $0)
      )
      (br $label$3)
     )
     (block $label$7
      (br_if $label$7
       (i32.eq
        (local.tee $4
         (i32.load
          (i32.add
           (local.get $0)
           (i32.const 12)
          )
         )
        )
        (local.tee $5
         (i32.load
          (i32.add
           (local.get $0)
           (i32.const 8)
          )
         )
        )
       )
      )
      (i32.store offset=12
       (local.get $5)
       (local.get $4)
      )
      (i32.store offset=8
       (local.get $4)
       (local.get $5)
      )
      (br $label$3)
     )
     (i32.store offset=1048624
      (i32.const 0)
      (i32.and
       (i32.load offset=1048624
        (i32.const 0)
       )
       (i32.rotl
        (i32.const -2)
        (i32.shr_u
         (local.get $3)
         (i32.const 3)
        )
       )
      )
     )
    )
    (block $label$8
     (br_if $label$8
      (i32.eqz
       (call $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E
        (local.get $2)
       )
      )
     )
     (call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
      (local.get $0)
      (local.get $1)
      (local.get $2)
     )
     (br $label$1)
    )
    (block $label$9
     (block $label$10
      (br_if $label$10
       (i32.eq
        (local.get $2)
        (i32.load offset=1049036
         (i32.const 0)
        )
       )
      )
      (br_if $label$9
       (i32.ne
        (local.get $2)
        (i32.load offset=1049032
         (i32.const 0)
        )
       )
      )
      (i32.store offset=1049032
       (i32.const 0)
       (local.get $0)
      )
      (i32.store offset=1049024
       (i32.const 0)
       (local.tee $1
        (i32.add
         (i32.load offset=1049024
          (i32.const 0)
         )
         (local.get $1)
        )
       )
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
       (local.get $0)
       (local.get $1)
      )
      (return)
     )
     (i32.store offset=1049036
      (i32.const 0)
      (local.get $0)
     )
     (i32.store offset=1049028
      (i32.const 0)
      (local.tee $1
       (i32.add
        (i32.load offset=1049028
         (i32.const 0)
        )
        (local.get $1)
       )
      )
     )
     (i32.store offset=4
      (local.get $0)
      (i32.or
       (local.get $1)
       (i32.const 1)
      )
     )
     (br_if $label$2
      (i32.ne
       (local.get $0)
       (i32.load offset=1049032
        (i32.const 0)
       )
      )
     )
     (i32.store offset=1049024
      (i32.const 0)
      (i32.const 0)
     )
     (i32.store offset=1049032
      (i32.const 0)
      (i32.const 0)
     )
     (return)
    )
    (local.set $1
     (i32.add
      (local.tee $3
       (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
        (local.get $2)
       )
      )
      (local.get $1)
     )
    )
    (block $label$11
     (block $label$12
      (br_if $label$12
       (i32.lt_u
        (local.get $3)
        (i32.const 256)
       )
      )
      (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
       (i32.const 1048624)
       (local.get $2)
      )
      (br $label$11)
     )
     (block $label$13
      (br_if $label$13
       (i32.eq
        (local.tee $4
         (i32.load
          (i32.add
           (local.get $2)
           (i32.const 12)
          )
         )
        )
        (local.tee $2
         (i32.load
          (i32.add
           (local.get $2)
           (i32.const 8)
          )
         )
        )
       )
      )
      (i32.store offset=12
       (local.get $2)
       (local.get $4)
      )
      (i32.store offset=8
       (local.get $4)
       (local.get $2)
      )
      (br $label$11)
     )
     (i32.store offset=1048624
      (i32.const 0)
      (i32.and
       (i32.load offset=1048624
        (i32.const 0)
       )
       (i32.rotl
        (i32.const -2)
        (i32.shr_u
         (local.get $3)
         (i32.const 3)
        )
       )
      )
     )
    )
    (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
     (local.get $0)
     (local.get $1)
    )
    (br_if $label$1
     (i32.ne
      (local.get $0)
      (i32.load offset=1049032
       (i32.const 0)
      )
     )
    )
    (i32.store offset=1049024
     (i32.const 0)
     (local.get $1)
    )
   )
   (return)
  )
  (block $label$14
   (br_if $label$14
    (i32.lt_u
     (local.get $1)
     (i32.const 256)
    )
   )
   (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
    (i32.const 1048624)
    (local.get $0)
    (local.get $1)
   )
   (return)
  )
  (local.set $1
   (i32.add
    (i32.shl
     (local.tee $2
      (i32.shr_u
       (local.get $1)
       (i32.const 3)
      )
     )
     (i32.const 3)
    )
    (i32.const 1048632)
   )
  )
  (block $label$15
   (block $label$16
    (br_if $label$16
     (i32.eqz
      (i32.and
       (local.tee $3
        (i32.load offset=1048624
         (i32.const 0)
        )
       )
       (local.tee $2
        (i32.shl
         (i32.const 1)
         (local.get $2)
        )
       )
      )
     )
    )
    (local.set $2
     (i32.load offset=8
      (local.get $1)
     )
    )
    (br $label$15)
   )
   (i32.store offset=1048624
    (i32.const 0)
    (i32.or
     (local.get $3)
     (local.get $2)
    )
   )
   (local.set $2
    (local.get $1)
   )
  )
  (i32.store offset=8
   (local.get $1)
   (local.get $0)
  )
  (i32.store offset=12
   (local.get $2)
   (local.get $0)
  )
  (i32.store offset=12
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=8
   (local.get $0)
   (local.get $2)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local.set $2
   (i32.load offset=24
    (local.get $1)
   )
  )
  (block $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.ne
       (call $_ZN8dlmalloc8dlmalloc9TreeChunk4next17h656f2e3867c8acf8E
        (local.get $1)
       )
       (local.get $1)
      )
     )
     (br_if $label$2
      (local.tee $5
       (i32.load
        (i32.add
         (local.get $1)
         (select
          (i32.const 20)
          (i32.const 16)
          (local.tee $4
           (i32.load
            (local.tee $3
             (i32.add
              (local.get $1)
              (i32.const 20)
             )
            )
           )
          )
         )
        )
       )
      )
     )
     (local.set $3
      (i32.const 0)
     )
     (br $label$1)
    )
    (i32.store offset=12
     (local.tee $5
      (call $_ZN8dlmalloc8dlmalloc9TreeChunk4prev17h527f673fd8318adbE
       (local.get $1)
      )
     )
     (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
      (local.tee $3
       (call $_ZN8dlmalloc8dlmalloc9TreeChunk4next17h656f2e3867c8acf8E
        (local.get $1)
       )
      )
     )
    )
    (i32.store offset=8
     (local.get $3)
     (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
      (local.get $5)
     )
    )
    (br $label$1)
   )
   (local.set $4
    (select
     (local.get $3)
     (i32.add
      (local.get $1)
      (i32.const 16)
     )
     (local.get $4)
    )
   )
   (loop $label$4
    (local.set $6
     (local.get $4)
    )
    (br_if $label$4
     (local.tee $5
      (i32.load
       (local.tee $4
        (select
         (local.tee $5
          (i32.add
           (local.tee $3
            (local.get $5)
           )
           (i32.const 20)
          )
         )
         (i32.add
          (local.get $3)
          (i32.const 16)
         )
         (i32.load
          (local.get $5)
         )
        )
       )
      )
     )
    )
   )
   (i32.store
    (local.get $6)
    (i32.const 0)
   )
  )
  (block $label$5
   (br_if $label$5
    (i32.eqz
     (local.get $2)
    )
   )
   (block $label$6
    (block $label$7
     (br_if $label$7
      (i32.eq
       (i32.load
        (local.tee $5
         (i32.add
          (i32.add
           (local.get $0)
           (i32.shl
            (local.tee $4
             (i32.load offset=28
              (local.get $1)
             )
            )
            (i32.const 2)
           )
          )
          (i32.const 272)
         )
        )
       )
       (local.get $1)
      )
     )
     (i32.store
      (i32.add
       (local.get $2)
       (select
        (i32.const 16)
        (i32.const 20)
        (i32.eq
         (i32.load offset=16
          (local.get $2)
         )
         (local.get $1)
        )
       )
      )
      (local.get $3)
     )
     (br_if $label$6
      (local.get $3)
     )
     (br $label$5)
    )
    (i32.store
     (local.get $5)
     (local.get $3)
    )
    (br_if $label$6
     (local.get $3)
    )
    (i32.store offset=4
     (local.get $0)
     (i32.and
      (i32.load offset=4
       (local.get $0)
      )
      (i32.rotl
       (i32.const -2)
       (local.get $4)
      )
     )
    )
    (return)
   )
   (i32.store offset=24
    (local.get $3)
    (local.get $2)
   )
   (block $label$8
    (br_if $label$8
     (i32.eqz
      (local.tee $5
       (i32.load offset=16
        (local.get $1)
       )
      )
     )
    )
    (i32.store offset=16
     (local.get $3)
     (local.get $5)
    )
    (i32.store offset=24
     (local.get $5)
     (local.get $3)
    )
   )
   (br_if $label$5
    (i32.eqz
     (local.tee $5
      (i32.load
       (i32.add
        (local.get $1)
        (i32.const 20)
       )
      )
     )
    )
   )
   (i32.store
    (i32.add
     (local.get $3)
     (i32.const 20)
    )
    (local.get $5)
   )
   (i32.store offset=24
    (local.get $5)
    (local.get $3)
   )
   (return)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local.set $3
   (i32.const 0)
  )
  (block $label$1
   (br_if $label$1
    (i32.lt_u
     (local.get $2)
     (i32.const 256)
    )
   )
   (local.set $3
    (i32.const 31)
   )
   (br_if $label$1
    (i32.gt_u
     (local.get $2)
     (i32.const 16777215)
    )
   )
   (local.set $3
    (i32.add
     (i32.sub
      (i32.and
       (i32.shr_u
        (local.get $2)
        (i32.sub
         (i32.const 6)
         (local.tee $3
          (i32.clz
           (i32.shr_u
            (local.get $2)
            (i32.const 8)
           )
          )
         )
        )
       )
       (i32.const 1)
      )
      (i32.shl
       (local.get $3)
       (i32.const 1)
      )
     )
     (i32.const 62)
    )
   )
  )
  (i64.store offset=16 align=4
   (local.get $1)
   (i64.const 0)
  )
  (i32.store offset=28
   (local.get $1)
   (local.get $3)
  )
  (local.set $4
   (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
    (local.get $1)
   )
  )
  (local.set $5
   (i32.add
    (i32.add
     (local.get $0)
     (i32.shl
      (local.get $3)
      (i32.const 2)
     )
    )
    (i32.const 272)
   )
  )
  (block $label$2
   (block $label$3
    (block $label$4
     (block $label$5
      (block $label$6
       (br_if $label$6
        (i32.eqz
         (i32.and
          (local.tee $6
           (i32.load
            (local.tee $0
             (i32.add
              (local.get $0)
              (i32.const 4)
             )
            )
           )
          )
          (local.tee $7
           (i32.shl
            (i32.const 1)
            (local.get $3)
           )
          )
         )
        )
       )
       (local.set $5
        (i32.load
         (local.get $5)
        )
       )
       (local.set $3
        (call $_ZN8dlmalloc8dlmalloc24leftshift_for_tree_index17hd789c537cab28411E
         (local.get $3)
        )
       )
       (br_if $label$5
        (i32.ne
         (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
          (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
           (local.get $5)
          )
         )
         (local.get $2)
        )
       )
       (local.set $3
        (local.get $5)
       )
       (br $label$4)
      )
      (i32.store
       (local.get $0)
       (i32.or
        (local.get $6)
        (local.get $7)
       )
      )
      (i32.store offset=24
       (local.get $1)
       (local.get $5)
      )
      (i32.store
       (local.get $5)
       (local.get $1)
      )
      (br $label$2)
     )
     (local.set $0
      (i32.shl
       (local.get $2)
       (local.get $3)
      )
     )
     (loop $label$7
      (br_if $label$3
       (i32.eqz
        (local.tee $3
         (i32.load
          (local.tee $6
           (i32.add
            (i32.add
             (local.get $5)
             (i32.and
              (i32.shr_u
               (local.get $0)
               (i32.const 29)
              )
              (i32.const 4)
             )
            )
            (i32.const 16)
           )
          )
         )
        )
       )
      )
      (local.set $0
       (i32.shl
        (local.get $0)
        (i32.const 1)
       )
      )
      (local.set $5
       (local.get $3)
      )
      (br_if $label$7
       (i32.ne
        (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
         (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
          (local.get $3)
         )
        )
        (local.get $2)
       )
      )
     )
    )
    (i32.store offset=12
     (local.tee $0
      (i32.load offset=8
       (local.tee $3
        (call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
         (local.get $3)
        )
       )
      )
     )
     (local.get $4)
    )
    (i32.store offset=8
     (local.get $3)
     (local.get $4)
    )
    (i32.store offset=12
     (local.get $4)
     (local.get $3)
    )
    (i32.store offset=8
     (local.get $4)
     (local.get $0)
    )
    (i32.store offset=24
     (local.get $1)
     (i32.const 0)
    )
    (return)
   )
   (i32.store
    (local.get $6)
    (local.get $1)
   )
   (i32.store offset=24
    (local.get $1)
    (local.get $5)
   )
  )
  (i32.store offset=8
   (local.get $4)
   (local.get $4)
  )
  (i32.store offset=12
   (local.get $4)
   (local.get $4)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$23release_unused_segments17h75b413e6a85f4b60E (param $0 i32) (result i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 i32)
  (local $11 i32)
  (local $12 i32)
  (local $13 i32)
  (block $label$1
   (br_if $label$1
    (local.tee $1
     (i32.load
      (i32.add
       (local.get $0)
       (i32.const 432)
      )
     )
    )
   )
   (i32.store offset=448
    (local.get $0)
    (i32.const 4095)
   )
   (return
    (i32.const 0)
   )
  )
  (local.set $2
   (i32.add
    (local.get $0)
    (i32.const 424)
   )
  )
  (local.set $3
   (i32.const 0)
  )
  (local.set $4
   (i32.const 0)
  )
  (loop $label$2
   (local.set $1
    (i32.load offset=8
     (local.tee $5
      (local.get $1)
     )
    )
   )
   (local.set $6
    (i32.load offset=4
     (local.get $5)
    )
   )
   (local.set $7
    (i32.load
     (local.get $5)
    )
   )
   (block $label$3
    (block $label$4
     (br_if $label$4
      (i32.eqz
       (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$16can_release_part17ha9587956c545036fE
        (local.get $0)
        (i32.shr_u
         (i32.load
          (i32.add
           (local.get $5)
           (i32.const 12)
          )
         )
         (i32.const 1)
        )
       )
      )
     )
     (br_if $label$4
      (call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
       (local.get $5)
      )
     )
     (local.set $9
      (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
       (local.tee $8
        (i32.add
         (local.get $7)
         (i32.sub
          (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
           (local.tee $8
            (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
             (local.get $7)
            )
           )
           (i32.const 8)
          )
          (local.get $8)
         )
        )
       )
      )
     )
     (local.set $11
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (local.tee $10
        (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
       )
       (i32.const 8)
      )
     )
     (local.set $12
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (i32.const 20)
       (i32.const 8)
      )
     )
     (local.set $13
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (i32.const 16)
       (i32.const 8)
      )
     )
     (br_if $label$4
      (call $_ZN8dlmalloc8dlmalloc5Chunk5inuse17h4d9d8a6e39f8aee5E
       (local.get $8)
      )
     )
     (br_if $label$4
      (i32.lt_u
       (i32.add
        (local.get $8)
        (local.get $9)
       )
       (i32.add
        (local.get $7)
        (i32.sub
         (i32.add
          (local.get $10)
          (local.get $6)
         )
         (i32.add
          (i32.add
           (local.get $11)
           (local.get $12)
          )
          (local.get $13)
         )
        )
       )
      )
     )
     (block $label$5
      (block $label$6
       (br_if $label$6
        (i32.eq
         (i32.load offset=408
          (local.get $0)
         )
         (local.get $8)
        )
       )
       (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
        (local.get $0)
        (local.get $8)
       )
       (br $label$5)
      )
      (i32.store offset=400
       (local.get $0)
       (i32.const 0)
      )
      (i32.store offset=408
       (local.get $0)
       (i32.const 0)
      )
     )
     (block $label$7
      (br_if $label$7
       (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E
        (local.get $0)
        (local.get $7)
        (local.get $6)
       )
      )
      (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
       (local.get $0)
       (local.get $8)
       (local.get $9)
      )
      (br $label$4)
     )
     (i32.store offset=416
      (local.get $0)
      (i32.sub
       (i32.load offset=416
        (local.get $0)
       )
       (local.get $6)
      )
     )
     (i32.store offset=8
      (local.get $2)
      (local.get $1)
     )
     (local.set $3
      (i32.add
       (local.get $6)
       (local.get $3)
      )
     )
     (br $label$3)
    )
    (local.set $2
     (local.get $5)
    )
   )
   (local.set $4
    (i32.add
     (local.get $4)
     (i32.const 1)
    )
   )
   (br_if $label$2
    (local.get $1)
   )
  )
  (i32.store offset=448
   (local.get $0)
   (select
    (local.get $4)
    (i32.const 4095)
    (i32.gt_u
     (local.get $4)
     (i32.const 4095)
    )
   )
  )
  (local.get $3)
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E (param $0 i32) (param $1 i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local.set $1
   (call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
    (local.get $1)
   )
  )
  (local.set $3
   (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    (local.get $1)
    (local.tee $2
     (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
      (local.get $1)
     )
    )
   )
  )
  (block $label$1
   (block $label$2
    (block $label$3
     (br_if $label$3
      (call $_ZN8dlmalloc8dlmalloc5Chunk6pinuse17h89f5f80c1a4cb95aE
       (local.get $1)
      )
     )
     (local.set $4
      (i32.load
       (local.get $1)
      )
     )
     (block $label$4
      (block $label$5
       (br_if $label$5
        (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
         (local.get $1)
        )
       )
       (local.set $2
        (i32.add
         (local.get $4)
         (local.get $2)
        )
       )
       (br_if $label$4
        (i32.ne
         (local.tee $1
          (call $_ZN8dlmalloc8dlmalloc5Chunk12minus_offset17h39dd10694c91288eE
           (local.get $1)
           (local.get $4)
          )
         )
         (i32.load offset=408
          (local.get $0)
         )
        )
       )
       (br_if $label$3
        (i32.ne
         (i32.and
          (i32.load offset=4
           (local.get $3)
          )
          (i32.const 3)
         )
         (i32.const 3)
        )
       )
       (i32.store offset=400
        (local.get $0)
        (local.get $2)
       )
       (call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
        (local.get $1)
        (local.get $2)
        (local.get $3)
       )
       (return)
      )
      (br_if $label$2
       (i32.eqz
        (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E
         (local.get $0)
         (i32.sub
          (local.get $1)
          (local.get $4)
         )
         (local.tee $1
          (i32.add
           (i32.add
            (local.get $4)
            (local.get $2)
           )
           (i32.const 16)
          )
         )
        )
       )
      )
      (i32.store offset=416
       (local.get $0)
       (i32.sub
        (i32.load offset=416
         (local.get $0)
        )
        (local.get $1)
       )
      )
      (return)
     )
     (block $label$6
      (br_if $label$6
       (i32.lt_u
        (local.get $4)
        (i32.const 256)
       )
      )
      (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
       (local.get $0)
       (local.get $1)
      )
      (br $label$3)
     )
     (block $label$7
      (br_if $label$7
       (i32.eq
        (local.tee $5
         (i32.load
          (i32.add
           (local.get $1)
           (i32.const 12)
          )
         )
        )
        (local.tee $6
         (i32.load
          (i32.add
           (local.get $1)
           (i32.const 8)
          )
         )
        )
       )
      )
      (i32.store offset=12
       (local.get $6)
       (local.get $5)
      )
      (i32.store offset=8
       (local.get $5)
       (local.get $6)
      )
      (br $label$3)
     )
     (i32.store
      (local.get $0)
      (i32.and
       (i32.load
        (local.get $0)
       )
       (i32.rotl
        (i32.const -2)
        (i32.shr_u
         (local.get $4)
         (i32.const 3)
        )
       )
      )
     )
    )
    (block $label$8
     (block $label$9
      (br_if $label$9
       (i32.eqz
        (call $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E
         (local.get $3)
        )
       )
      )
      (call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
       (local.get $1)
       (local.get $2)
       (local.get $3)
      )
      (br $label$8)
     )
     (block $label$10
      (block $label$11
       (block $label$12
        (block $label$13
         (br_if $label$13
          (i32.eq
           (local.get $3)
           (i32.load offset=412
            (local.get $0)
           )
          )
         )
         (br_if $label$12
          (i32.ne
           (local.get $3)
           (i32.load offset=408
            (local.get $0)
           )
          )
         )
         (i32.store offset=408
          (local.get $0)
          (local.get $1)
         )
         (i32.store offset=400
          (local.get $0)
          (local.tee $2
           (i32.add
            (i32.load offset=400
             (local.get $0)
            )
            (local.get $2)
           )
          )
         )
         (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
          (local.get $1)
          (local.get $2)
         )
         (return)
        )
        (i32.store offset=412
         (local.get $0)
         (local.get $1)
        )
        (i32.store offset=404
         (local.get $0)
         (local.tee $2
          (i32.add
           (i32.load offset=404
            (local.get $0)
           )
           (local.get $2)
          )
         )
        )
        (i32.store offset=4
         (local.get $1)
         (i32.or
          (local.get $2)
          (i32.const 1)
         )
        )
        (br_if $label$11
         (i32.eq
          (local.get $1)
          (i32.load offset=408
           (local.get $0)
          )
         )
        )
        (br $label$10)
       )
       (local.set $2
        (i32.add
         (local.tee $4
          (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
           (local.get $3)
          )
         )
         (local.get $2)
        )
       )
       (block $label$14
        (block $label$15
         (br_if $label$15
          (i32.lt_u
           (local.get $4)
           (i32.const 256)
          )
         )
         (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
          (local.get $0)
          (local.get $3)
         )
         (br $label$14)
        )
        (block $label$16
         (br_if $label$16
          (i32.eq
           (local.tee $5
            (i32.load
             (i32.add
              (local.get $3)
              (i32.const 12)
             )
            )
           )
           (local.tee $3
            (i32.load
             (i32.add
              (local.get $3)
              (i32.const 8)
             )
            )
           )
          )
         )
         (i32.store offset=12
          (local.get $3)
          (local.get $5)
         )
         (i32.store offset=8
          (local.get $5)
          (local.get $3)
         )
         (br $label$14)
        )
        (i32.store
         (local.get $0)
         (i32.and
          (i32.load
           (local.get $0)
          )
          (i32.rotl
           (i32.const -2)
           (i32.shr_u
            (local.get $4)
            (i32.const 3)
           )
          )
         )
        )
       )
       (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
        (local.get $1)
        (local.get $2)
       )
       (br_if $label$8
        (i32.ne
         (local.get $1)
         (i32.load offset=408
          (local.get $0)
         )
        )
       )
       (i32.store offset=400
        (local.get $0)
        (local.get $2)
       )
       (br $label$2)
      )
      (i32.store offset=400
       (local.get $0)
       (i32.const 0)
      )
      (i32.store offset=408
       (local.get $0)
       (i32.const 0)
      )
     )
     (br_if $label$2
      (i32.ge_u
       (i32.load
        (i32.add
         (local.get $0)
         (i32.const 440)
        )
       )
       (local.get $2)
      )
     )
     (local.set $1
      (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
     )
     (br_if $label$2
      (i32.eqz
       (select
        (local.tee $1
         (i32.add
          (i32.and
           (i32.add
            (i32.sub
             (local.get $1)
             (i32.add
              (i32.add
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (local.get $1)
                (i32.const 8)
               )
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (i32.const 20)
                (i32.const 8)
               )
              )
              (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
               (i32.const 16)
               (i32.const 8)
              )
             )
            )
            (i32.const -65544)
           )
           (i32.const -9)
          )
          (i32.const -3)
         )
        )
        (local.tee $2
         (i32.sub
          (i32.const 0)
          (i32.shl
           (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            (i32.const 16)
            (i32.const 8)
           )
           (i32.const 2)
          )
         )
        )
        (i32.gt_u
         (local.get $2)
         (local.get $1)
        )
       )
      )
     )
     (br_if $label$2
      (i32.eqz
       (local.tee $2
        (i32.load offset=412
         (local.get $0)
        )
       )
      )
     )
     (local.set $3
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (local.tee $1
        (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
       )
       (i32.const 8)
      )
     )
     (local.set $5
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (i32.const 20)
       (i32.const 8)
      )
     )
     (local.set $6
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (i32.const 16)
       (i32.const 8)
      )
     )
     (local.set $4
      (i32.const 0)
     )
     (block $label$17
      (br_if $label$17
       (i32.le_u
        (local.tee $7
         (i32.load offset=404
          (local.get $0)
         )
        )
        (local.tee $1
         (i32.add
          (local.get $6)
          (i32.add
           (local.get $5)
           (i32.sub
            (local.get $3)
            (local.get $1)
           )
          )
         )
        )
       )
      )
      (local.set $5
       (i32.and
        (i32.add
         (local.get $7)
         (i32.xor
          (local.get $1)
          (i32.const -1)
         )
        )
        (i32.const -65536)
       )
      )
      (local.set $1
       (local.tee $3
        (i32.add
         (local.get $0)
         (i32.const 424)
        )
       )
      )
      (block $label$18
       (loop $label$19
        (block $label$20
         (br_if $label$20
          (i32.gt_u
           (i32.load
            (local.get $1)
           )
           (local.get $2)
          )
         )
         (br_if $label$18
          (i32.gt_u
           (call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
            (local.get $1)
           )
           (local.get $2)
          )
         )
        )
        (br_if $label$19
         (local.tee $1
          (i32.load offset=8
           (local.get $1)
          )
         )
        )
       )
       (local.set $1
        (i32.const 0)
       )
      )
      (local.set $4
       (i32.const 0)
      )
      (br_if $label$17
       (call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
        (local.get $1)
       )
      )
      (local.set $4
       (i32.const 0)
      )
      (br_if $label$17
       (i32.eqz
        (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$16can_release_part17ha9587956c545036fE
         (local.get $0)
         (i32.shr_u
          (i32.load
           (i32.add
            (local.get $1)
            (i32.const 12)
           )
          )
          (i32.const 1)
         )
        )
       )
      )
      (local.set $4
       (i32.const 0)
      )
      (br_if $label$17
       (i32.lt_u
        (i32.load offset=4
         (local.get $1)
        )
        (local.get $5)
       )
      )
      (loop $label$21
       (block $label$22
        (br_if $label$22
         (i32.eqz
          (call $_ZN8dlmalloc8dlmalloc7Segment5holds17h276a4b63e2947208E
           (local.get $1)
           (local.get $3)
          )
         )
        )
        (local.set $4
         (i32.const 0)
        )
        (br $label$17)
       )
       (br_if $label$21
        (local.tee $3
         (i32.load offset=8
          (local.get $3)
         )
        )
       )
      )
      (local.set $4
       (i32.const 0)
      )
      (br_if $label$17
       (i32.eqz
        (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9free_part17hfe7db7a0188c71a3E
         (local.get $0)
         (i32.load
          (local.get $1)
         )
         (local.tee $2
          (i32.load offset=4
           (local.get $1)
          )
         )
         (i32.sub
          (local.get $2)
          (local.get $5)
         )
        )
       )
      )
      (local.set $4
       (i32.const 0)
      )
      (br_if $label$17
       (i32.eqz
        (local.get $5)
       )
      )
      (i32.store offset=4
       (local.get $1)
       (i32.sub
        (i32.load offset=4
         (local.get $1)
        )
        (local.get $5)
       )
      )
      (i32.store offset=416
       (local.get $0)
       (i32.sub
        (i32.load offset=416
         (local.get $0)
        )
        (local.get $5)
       )
      )
      (local.set $2
       (i32.load offset=404
        (local.get $0)
       )
      )
      (local.set $1
       (i32.load offset=412
        (local.get $0)
       )
      )
      (i32.store offset=412
       (local.get $0)
       (local.tee $1
        (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
         (local.get $1)
         (local.tee $3
          (i32.sub
           (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            (local.tee $3
             (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
              (local.get $1)
             )
            )
            (i32.const 8)
           )
           (local.get $3)
          )
         )
        )
       )
      )
      (i32.store offset=404
       (local.get $0)
       (local.tee $2
        (i32.sub
         (local.get $2)
         (i32.add
          (local.get $5)
          (local.get $3)
         )
        )
       )
      )
      (i32.store offset=4
       (local.get $1)
       (i32.or
        (local.get $2)
        (i32.const 1)
       )
      )
      (local.set $4
       (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        (local.tee $3
         (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
        )
        (i32.const 8)
       )
      )
      (local.set $6
       (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        (i32.const 20)
        (i32.const 8)
       )
      )
      (local.set $7
       (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        (i32.const 16)
        (i32.const 8)
       )
      )
      (local.set $1
       (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
        (local.get $1)
        (local.get $2)
       )
      )
      (i32.store
       (i32.add
        (local.get $0)
        (i32.const 440)
       )
       (i32.const 2097152)
      )
      (i32.store offset=4
       (local.get $1)
       (i32.add
        (local.get $7)
        (i32.add
         (local.get $6)
         (i32.sub
          (local.get $4)
          (local.get $3)
         )
        )
       )
      )
      (local.set $4
       (local.get $5)
      )
     )
     (br_if $label$2
      (i32.ne
       (local.get $4)
       (i32.sub
        (i32.const 0)
        (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$23release_unused_segments17h75b413e6a85f4b60E
         (local.get $0)
        )
       )
      )
     )
     (br_if $label$2
      (i32.le_u
       (i32.load offset=404
        (local.get $0)
       )
       (i32.load
        (i32.add
         (local.get $0)
         (i32.const 440)
        )
       )
      )
     )
     (i32.store
      (i32.add
       (local.get $0)
       (i32.const 440)
      )
      (i32.const -1)
     )
     (return)
    )
    (br_if $label$1
     (i32.lt_u
      (local.get $2)
      (i32.const 256)
     )
    )
    (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
     (local.get $0)
     (local.get $1)
     (local.get $2)
    )
    (i32.store offset=448
     (local.get $0)
     (local.tee $1
      (i32.add
       (i32.load offset=448
        (local.get $0)
       )
       (i32.const -1)
      )
     )
    )
    (br_if $label$2
     (local.get $1)
    )
    (drop
     (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$23release_unused_segments17h75b413e6a85f4b60E
      (local.get $0)
     )
    )
    (return)
   )
   (return)
  )
  (local.set $2
   (i32.add
    (i32.add
     (local.get $0)
     (i32.shl
      (local.tee $3
       (i32.shr_u
        (local.get $2)
        (i32.const 3)
       )
      )
      (i32.const 3)
     )
    )
    (i32.const 8)
   )
  )
  (block $label$23
   (block $label$24
    (br_if $label$24
     (i32.eqz
      (i32.and
       (local.tee $4
        (i32.load
         (local.get $0)
        )
       )
       (local.tee $3
        (i32.shl
         (i32.const 1)
         (local.get $3)
        )
       )
      )
     )
    )
    (local.set $0
     (i32.load offset=8
      (local.get $2)
     )
    )
    (br $label$23)
   )
   (i32.store
    (local.get $0)
    (i32.or
     (local.get $4)
     (local.get $3)
    )
   )
   (local.set $0
    (local.get $2)
   )
  )
  (i32.store offset=8
   (local.get $2)
   (local.get $1)
  )
  (i32.store offset=12
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=12
   (local.get $1)
   (local.get $2)
  )
  (i32.store offset=8
   (local.get $1)
   (local.get $0)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$8init_top17h9ba4d179485fee16E (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local.set $1
   (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    (local.get $1)
    (local.tee $3
     (i32.sub
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (local.tee $3
        (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
         (local.get $1)
        )
       )
       (i32.const 8)
      )
      (local.get $3)
     )
    )
   )
  )
  (i32.store offset=404
   (local.get $0)
   (local.tee $2
    (i32.sub
     (local.get $2)
     (local.get $3)
    )
   )
  )
  (i32.store offset=412
   (local.get $0)
   (local.get $1)
  )
  (i32.store offset=4
   (local.get $1)
   (i32.or
    (local.get $2)
    (i32.const 1)
   )
  )
  (local.set $4
   (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    (local.tee $3
     (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
    )
    (i32.const 8)
   )
  )
  (local.set $5
   (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    (i32.const 20)
    (i32.const 8)
   )
  )
  (local.set $6
   (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    (i32.const 16)
    (i32.const 8)
   )
  )
  (local.set $1
   (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    (local.get $1)
    (local.get $2)
   )
  )
  (i32.store offset=440
   (local.get $0)
   (i32.const 2097152)
  )
  (i32.store offset=4
   (local.get $1)
   (i32.add
    (local.get $6)
    (i32.add
     (local.get $5)
     (i32.sub
      (local.get $4)
      (local.get $3)
     )
    )
   )
  )
 )
 (func $_ZN3std7process5abort17hf7c8bef35d3938e7E
  (unreachable)
 )
 (func $__rdl_alloc (param $0 i32) (param $1 i32) (result i32)
  (call $_ZN8dlmalloc17Dlmalloc$LT$A$GT$6malloc17he4572c35964f8c9bE
   (local.get $0)
   (local.get $1)
  )
 )
 (func $__rdl_dealloc (param $0 i32) (param $1 i32) (param $2 i32)
  (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E
   (i32.const 1048624)
   (local.get $0)
  )
 )
 (func $__rdl_realloc (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (result i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (block $label$1
   (block $label$2
    (block $label$3
     (block $label$4
      (br_if $label$4
       (i32.lt_u
        (local.get $2)
        (i32.const 9)
       )
      )
      (br_if $label$3
       (local.tee $2
        (call $_ZN8dlmalloc17Dlmalloc$LT$A$GT$6malloc17he4572c35964f8c9bE
         (local.get $3)
         (local.get $2)
        )
       )
      )
      (return
       (i32.const 0)
      )
     )
     (local.set $1
      (call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE)
     )
     (local.set $2
      (i32.const 0)
     )
     (br_if $label$2
      (i32.le_u
       (select
        (local.tee $1
         (i32.add
          (i32.and
           (i32.add
            (i32.sub
             (local.get $1)
             (i32.add
              (i32.add
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (local.get $1)
                (i32.const 8)
               )
               (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                (i32.const 20)
                (i32.const 8)
               )
              )
              (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
               (i32.const 16)
               (i32.const 8)
              )
             )
            )
            (i32.const -65544)
           )
           (i32.const -9)
          )
          (i32.const -3)
         )
        )
        (local.tee $4
         (i32.sub
          (i32.const 0)
          (i32.shl
           (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            (i32.const 16)
            (i32.const 8)
           )
           (i32.const 2)
          )
         )
        )
        (i32.gt_u
         (local.get $4)
         (local.get $1)
        )
       )
       (local.get $3)
      )
     )
     (local.set $4
      (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
       (select
        (i32.const 16)
        (i32.add
         (local.get $3)
         (i32.const 4)
        )
        (i32.gt_u
         (i32.add
          (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
           (i32.const 16)
           (i32.const 8)
          )
          (i32.const -5)
         )
         (local.get $3)
        )
       )
       (i32.const 8)
      )
     )
     (local.set $1
      (call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
       (local.get $0)
      )
     )
     (local.set $6
      (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
       (local.get $1)
       (local.tee $5
        (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
         (local.get $1)
        )
       )
      )
     )
     (block $label$5
      (block $label$6
       (block $label$7
        (block $label$8
         (block $label$9
          (block $label$10
           (block $label$11
            (block $label$12
             (br_if $label$12
              (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
               (local.get $1)
              )
             )
             (br_if $label$11
              (i32.ge_u
               (local.get $5)
               (local.get $4)
              )
             )
             (br_if $label$10
              (i32.eq
               (local.get $6)
               (i32.load offset=1049036
                (i32.const 0)
               )
              )
             )
             (br_if $label$9
              (i32.eq
               (local.get $6)
               (i32.load offset=1049032
                (i32.const 0)
               )
              )
             )
             (br_if $label$5
              (call $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E
               (local.get $6)
              )
             )
             (br_if $label$5
              (i32.lt_u
               (local.tee $5
                (i32.add
                 (local.tee $7
                  (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                   (local.get $6)
                  )
                 )
                 (local.get $5)
                )
               )
               (local.get $4)
              )
             )
             (local.set $8
              (i32.sub
               (local.get $5)
               (local.get $4)
              )
             )
             (br_if $label$8
              (i32.lt_u
               (local.get $7)
               (i32.const 256)
              )
             )
             (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
              (i32.const 1048624)
              (local.get $6)
             )
             (br $label$7)
            )
            (local.set $5
             (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
              (local.get $1)
             )
            )
            (br_if $label$5
             (i32.lt_u
              (local.get $4)
              (i32.const 256)
             )
            )
            (block $label$13
             (br_if $label$13
              (i32.lt_u
               (local.get $5)
               (i32.add
                (local.get $4)
                (i32.const 4)
               )
              )
             )
             (br_if $label$6
              (i32.lt_u
               (i32.sub
                (local.get $5)
                (local.get $4)
               )
               (i32.const 131073)
              )
             )
            )
            (br_if $label$5
             (i32.eqz
              (local.tee $4
               (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5remap17h737373babb5822ceE
                (i32.const 1048624)
                (i32.sub
                 (local.get $1)
                 (local.tee $6
                  (i32.load
                   (local.get $1)
                  )
                 )
                )
                (local.tee $7
                 (i32.add
                  (i32.add
                   (local.get $5)
                   (local.get $6)
                  )
                  (i32.const 16)
                 )
                )
                (local.tee $5
                 (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  (i32.add
                   (local.get $4)
                   (i32.const 31)
                  )
                  (call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9page_size17hf5189f015a43cc18E
                   (i32.const 1048624)
                  )
                 )
                )
                (i32.const 1)
               )
              )
             )
            )
            (i32.store offset=4
             (local.tee $1
              (i32.add
               (local.get $4)
               (local.get $6)
              )
             )
             (local.tee $2
              (i32.add
               (local.tee $3
                (i32.sub
                 (local.get $5)
                 (local.get $6)
                )
               )
               (i32.const -16)
              )
             )
            )
            (local.set $0
             (call $_ZN8dlmalloc8dlmalloc5Chunk14fencepost_head17h32cfaa035be31489E)
            )
            (i32.store offset=4
             (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
              (local.get $1)
              (local.get $2)
             )
             (local.get $0)
            )
            (i32.store offset=4
             (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
              (local.get $1)
              (i32.add
               (local.get $3)
               (i32.const -12)
              )
             )
             (i32.const 0)
            )
            (i32.store offset=1049040
             (i32.const 0)
             (local.tee $3
              (i32.add
               (i32.load offset=1049040
                (i32.const 0)
               )
               (i32.sub
                (local.get $5)
                (local.get $7)
               )
              )
             )
            )
            (i32.store offset=1049068
             (i32.const 0)
             (select
              (local.tee $2
               (i32.load offset=1049068
                (i32.const 0)
               )
              )
              (local.get $4)
              (i32.gt_u
               (local.get $4)
               (local.get $2)
              )
             )
            )
            (i32.store offset=1049044
             (i32.const 0)
             (select
              (local.tee $2
               (i32.load offset=1049044
                (i32.const 0)
               )
              )
              (local.get $3)
              (i32.gt_u
               (local.get $2)
               (local.get $3)
              )
             )
            )
            (br $label$1)
           )
           (br_if $label$6
            (i32.lt_u
             (local.tee $5
              (i32.sub
               (local.get $5)
               (local.get $4)
              )
             )
             (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
              (i32.const 16)
              (i32.const 8)
             )
            )
           )
           (local.set $6
            (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
             (local.get $1)
             (local.get $4)
            )
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
            (local.get $1)
            (local.get $4)
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
            (local.get $6)
            (local.get $5)
           )
           (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
            (local.get $6)
            (local.get $5)
           )
           (br $label$6)
          )
          (br_if $label$5
           (i32.le_u
            (local.tee $5
             (i32.add
              (i32.load offset=1049028
               (i32.const 0)
              )
              (local.get $5)
             )
            )
            (local.get $4)
           )
          )
          (local.set $6
           (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
            (local.get $1)
            (local.get $4)
           )
          )
          (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
           (local.get $1)
           (local.get $4)
          )
          (i32.store offset=4
           (local.get $6)
           (i32.or
            (local.tee $4
             (i32.sub
              (local.get $5)
              (local.get $4)
             )
            )
            (i32.const 1)
           )
          )
          (i32.store offset=1049028
           (i32.const 0)
           (local.get $4)
          )
          (i32.store offset=1049036
           (i32.const 0)
           (local.get $6)
          )
          (br $label$6)
         )
         (br_if $label$5
          (i32.lt_u
           (local.tee $5
            (i32.add
             (i32.load offset=1049024
              (i32.const 0)
             )
             (local.get $5)
            )
           )
           (local.get $4)
          )
         )
         (block $label$14
          (block $label$15
           (br_if $label$15
            (i32.ge_u
             (local.tee $6
              (i32.sub
               (local.get $5)
               (local.get $4)
              )
             )
             (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
              (i32.const 16)
              (i32.const 8)
             )
            )
           )
           (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
            (local.get $1)
            (local.get $5)
           )
           (local.set $6
            (i32.const 0)
           )
           (local.set $5
            (i32.const 0)
           )
           (br $label$14)
          )
          (local.set $7
           (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
            (local.tee $5
             (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
              (local.get $1)
              (local.get $4)
             )
            )
            (local.get $6)
           )
          )
          (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
           (local.get $1)
           (local.get $4)
          )
          (call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
           (local.get $5)
           (local.get $6)
          )
          (call $_ZN8dlmalloc8dlmalloc5Chunk12clear_pinuse17h2a67940d6f74a782E
           (local.get $7)
          )
         )
         (i32.store offset=1049032
          (i32.const 0)
          (local.get $5)
         )
         (i32.store offset=1049024
          (i32.const 0)
          (local.get $6)
         )
         (br $label$6)
        )
        (block $label$16
         (br_if $label$16
          (i32.eq
           (local.tee $9
            (i32.load
             (i32.add
              (local.get $6)
              (i32.const 12)
             )
            )
           )
           (local.tee $6
            (i32.load
             (i32.add
              (local.get $6)
              (i32.const 8)
             )
            )
           )
          )
         )
         (i32.store offset=12
          (local.get $6)
          (local.get $9)
         )
         (i32.store offset=8
          (local.get $9)
          (local.get $6)
         )
         (br $label$7)
        )
        (i32.store offset=1048624
         (i32.const 0)
         (i32.and
          (i32.load offset=1048624
           (i32.const 0)
          )
          (i32.rotl
           (i32.const -2)
           (i32.shr_u
            (local.get $7)
            (i32.const 3)
           )
          )
         )
        )
       )
       (block $label$17
        (br_if $label$17
         (i32.lt_u
          (local.get $8)
          (call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
           (i32.const 16)
           (i32.const 8)
          )
         )
        )
        (local.set $5
         (call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
          (local.get $1)
          (local.get $4)
         )
        )
        (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
         (local.get $1)
         (local.get $4)
        )
        (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
         (local.get $5)
         (local.get $8)
        )
        (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
         (local.get $5)
         (local.get $8)
        )
        (br $label$6)
       )
       (call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
        (local.get $1)
        (local.get $5)
       )
      )
      (br_if $label$1
       (local.get $1)
      )
     )
     (br_if $label$2
      (i32.eqz
       (local.tee $4
        (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E
         (local.get $3)
        )
       )
      )
     )
     (local.set $3
      (call $memcpy
       (local.get $4)
       (local.get $0)
       (select
        (local.get $3)
        (local.tee $2
         (i32.add
          (call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
           (local.get $1)
          )
          (select
           (i32.const -8)
           (i32.const -4)
           (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
            (local.get $1)
           )
          )
         )
        )
        (i32.gt_u
         (local.get $2)
         (local.get $3)
        )
       )
      )
     )
     (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E
      (i32.const 1048624)
      (local.get $0)
     )
     (return
      (local.get $3)
     )
    )
    (drop
     (call $memcpy
      (local.get $2)
      (local.get $0)
      (select
       (local.get $3)
       (local.get $1)
       (i32.gt_u
        (local.get $1)
        (local.get $3)
       )
      )
     )
    )
    (call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E
     (i32.const 1048624)
     (local.get $0)
    )
   )
   (return
    (local.get $2)
   )
  )
  (drop
   (call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
    (local.get $1)
   )
  )
  (call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
   (local.get $1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E (param $0 i32) (param $1 i32) (result i32)
  (i32.and
   (i32.add
    (i32.add
     (local.get $0)
     (local.get $1)
    )
    (i32.const -1)
   )
   (i32.sub
    (i32.const 0)
    (local.get $1)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc9left_bits17hd43e75bebd2d32bdE (param $0 i32) (result i32)
  (i32.or
   (local.tee $0
    (i32.shl
     (local.get $0)
     (i32.const 1)
    )
   )
   (i32.sub
    (i32.const 0)
    (local.get $0)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE (param $0 i32) (result i32)
  (i32.and
   (i32.sub
    (i32.const 0)
    (local.get $0)
   )
   (local.get $0)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc24leftshift_for_tree_index17hd789c537cab28411E (param $0 i32) (result i32)
  (select
   (i32.const 0)
   (i32.sub
    (i32.const 25)
    (i32.shr_u
     (local.get $0)
     (i32.const 1)
    )
   )
   (i32.eq
    (local.get $0)
    (i32.const 31)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk14fencepost_head17h32cfaa035be31489E (result i32)
  (i32.const 7)
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E (param $0 i32) (result i32)
  (i32.and
   (i32.load offset=4
    (local.get $0)
   )
   (i32.const -8)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E (param $0 i32) (result i32)
  (i32.shr_u
   (i32.and
    (i32.load8_u offset=4
     (local.get $0)
    )
    (i32.const 2)
   )
   (i32.const 1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk6pinuse17h89f5f80c1a4cb95aE (param $0 i32) (result i32)
  (i32.and
   (i32.load offset=4
    (local.get $0)
   )
   (i32.const 1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk12clear_pinuse17h2a67940d6f74a782E (param $0 i32)
  (i32.store offset=4
   (local.get $0)
   (i32.and
    (i32.load offset=4
     (local.get $0)
    )
    (i32.const -2)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk5inuse17h4d9d8a6e39f8aee5E (param $0 i32) (result i32)
  (i32.ne
   (i32.and
    (i32.load offset=4
     (local.get $0)
    )
    (i32.const 3)
   )
   (i32.const 1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE (param $0 i32) (result i32)
  (i32.eqz
   (i32.and
    (i32.load8_u offset=4
     (local.get $0)
    )
    (i32.const 3)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E (param $0 i32) (param $1 i32)
  (i32.store offset=4
   (local.get $0)
   (i32.or
    (i32.or
     (i32.and
      (i32.load offset=4
       (local.get $0)
      )
      (i32.const 1)
     )
     (local.get $1)
    )
    (i32.const 2)
   )
  )
  (i32.store offset=4
   (local.tee $0
    (i32.add
     (local.get $0)
     (local.get $1)
    )
   )
   (i32.or
    (i32.load offset=4
     (local.get $0)
    )
    (i32.const 1)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E (param $0 i32) (param $1 i32)
  (i32.store offset=4
   (local.get $0)
   (i32.or
    (local.get $1)
    (i32.const 3)
   )
  )
  (i32.store offset=4
   (local.tee $0
    (i32.add
     (local.get $0)
     (local.get $1)
    )
   )
   (i32.or
    (i32.load offset=4
     (local.get $0)
    )
    (i32.const 1)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E (param $0 i32) (param $1 i32)
  (i32.store offset=4
   (local.get $0)
   (i32.or
    (local.get $1)
    (i32.const 3)
   )
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E (param $0 i32) (param $1 i32)
  (i32.store offset=4
   (local.get $0)
   (i32.or
    (local.get $1)
    (i32.const 1)
   )
  )
  (i32.store
   (i32.add
    (local.get $0)
    (local.get $1)
   )
   (local.get $1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E (param $0 i32) (param $1 i32) (param $2 i32)
  (i32.store offset=4
   (local.get $2)
   (i32.and
    (i32.load offset=4
     (local.get $2)
    )
    (i32.const -2)
   )
  )
  (i32.store offset=4
   (local.get $0)
   (i32.or
    (local.get $1)
    (i32.const 1)
   )
  )
  (i32.store
   (i32.add
    (local.get $0)
    (local.get $1)
   )
   (local.get $1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E (param $0 i32) (param $1 i32) (result i32)
  (i32.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk12minus_offset17h39dd10694c91288eE (param $0 i32) (param $1 i32) (result i32)
  (i32.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE (param $0 i32) (result i32)
  (i32.add
   (local.get $0)
   (i32.const 8)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE (result i32)
  (i32.const 8)
 )
 (func $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E (param $0 i32) (result i32)
  (i32.add
   (local.get $0)
   (i32.const -8)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE (param $0 i32) (result i32)
  (local $1 i32)
  (block $label$1
   (br_if $label$1
    (local.tee $1
     (i32.load offset=16
      (local.get $0)
     )
    )
   )
   (local.set $1
    (i32.load
     (i32.add
      (local.get $0)
      (i32.const 20)
     )
    )
   )
  )
  (local.get $1)
 )
 (func $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE (param $0 i32) (result i32)
  (local.get $0)
 )
 (func $_ZN8dlmalloc8dlmalloc9TreeChunk4next17h656f2e3867c8acf8E (param $0 i32) (result i32)
  (i32.load offset=12
   (local.get $0)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc9TreeChunk4prev17h527f673fd8318adbE (param $0 i32) (result i32)
  (i32.load offset=8
   (local.get $0)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E (param $0 i32) (result i32)
  (i32.and
   (i32.load offset=12
    (local.get $0)
   )
   (i32.const 1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc7Segment9sys_flags17h6d168430a1d92f9aE (param $0 i32) (result i32)
  (i32.shr_u
   (i32.load offset=12
    (local.get $0)
   )
   (i32.const 1)
  )
 )
 (func $_ZN8dlmalloc8dlmalloc7Segment5holds17h276a4b63e2947208E (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local.set $2
   (i32.const 0)
  )
  (block $label$1
   (br_if $label$1
    (i32.gt_u
     (local.tee $3
      (i32.load
       (local.get $0)
      )
     )
     (local.get $1)
    )
   )
   (local.set $2
    (i32.gt_u
     (i32.add
      (local.get $3)
      (i32.load offset=4
       (local.get $0)
      )
     )
     (local.get $1)
    )
   )
  )
  (local.get $2)
 )
 (func $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E (param $0 i32) (result i32)
  (i32.add
   (i32.load
    (local.get $0)
   )
   (i32.load offset=4
    (local.get $0)
   )
  )
 )
 (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5alloc17he1272ca423b0b1b4E (param $0 i32) (param $1 i32) (param $2 i32)
  (local $3 i32)
  (local.set $3
   (memory.grow
    (i32.shr_u
     (local.get $2)
     (i32.const 16)
    )
   )
  )
  (i32.store offset=8
   (local.get $0)
   (i32.const 0)
  )
  (i32.store offset=4
   (local.get $0)
   (select
    (i32.const 0)
    (i32.and
     (local.get $2)
     (i32.const -65536)
    )
    (local.tee $2
     (i32.eq
      (local.get $3)
      (i32.const -1)
     )
    )
   )
  )
  (i32.store
   (local.get $0)
   (select
    (i32.const 0)
    (i32.shl
     (local.get $3)
     (i32.const 16)
    )
    (local.get $2)
   )
  )
 )
 (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5remap17h737373babb5822ceE (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (result i32)
  (i32.const 0)
 )
 (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9free_part17hfe7db7a0188c71a3E (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (result i32)
  (i32.const 0)
 )
 (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (i32.const 0)
 )
 (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$16can_release_part17ha9587956c545036fE (param $0 i32) (param $1 i32) (result i32)
  (i32.const 0)
 )
 (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9page_size17hf5189f015a43cc18E (param $0 i32) (result i32)
  (i32.const 65536)
 )
 (func $memcpy (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (call $_ZN17compiler_builtins3mem6memcpy17h7097b81567cf1b82E
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
 )
 (func $_ZN17compiler_builtins3mem6memcpy17h7097b81567cf1b82E (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 i32)
  (block $label$1
   (block $label$2
    (br_if $label$2
     (i32.gt_u
      (local.get $2)
      (i32.const 15)
     )
    )
    (local.set $3
     (local.get $0)
    )
    (br $label$1)
   )
   (local.set $5
    (i32.add
     (local.get $0)
     (local.tee $4
      (i32.and
       (i32.sub
        (i32.const 0)
        (local.get $0)
       )
       (i32.const 3)
      )
     )
    )
   )
   (block $label$3
    (br_if $label$3
     (i32.eqz
      (local.get $4)
     )
    )
    (local.set $3
     (local.get $0)
    )
    (local.set $6
     (local.get $1)
    )
    (loop $label$4
     (i32.store8
      (local.get $3)
      (i32.load8_u
       (local.get $6)
      )
     )
     (local.set $6
      (i32.add
       (local.get $6)
       (i32.const 1)
      )
     )
     (br_if $label$4
      (i32.lt_u
       (local.tee $3
        (i32.add
         (local.get $3)
         (i32.const 1)
        )
       )
       (local.get $5)
      )
     )
    )
   )
   (local.set $3
    (i32.add
     (local.get $5)
     (local.tee $8
      (i32.and
       (local.tee $7
        (i32.sub
         (local.get $2)
         (local.get $4)
        )
       )
       (i32.const -4)
      )
     )
    )
   )
   (block $label$5
    (block $label$6
     (br_if $label$6
      (i32.eqz
       (local.tee $6
        (i32.and
         (local.tee $9
          (i32.add
           (local.get $1)
           (local.get $4)
          )
         )
         (i32.const 3)
        )
       )
      )
     )
     (br_if $label$5
      (i32.lt_s
       (local.get $8)
       (i32.const 1)
      )
     )
     (local.set $1
      (i32.add
       (local.tee $10
        (i32.and
         (local.get $9)
         (i32.const -4)
        )
       )
       (i32.const 4)
      )
     )
     (local.set $4
      (i32.and
       (i32.sub
        (i32.const 0)
        (local.tee $2
         (i32.shl
          (local.get $6)
          (i32.const 3)
         )
        )
       )
       (i32.const 24)
      )
     )
     (local.set $6
      (i32.load
       (local.get $10)
      )
     )
     (loop $label$7
      (i32.store
       (local.get $5)
       (i32.or
        (i32.shr_u
         (local.get $6)
         (local.get $2)
        )
        (i32.shl
         (local.tee $6
          (i32.load
           (local.get $1)
          )
         )
         (local.get $4)
        )
       )
      )
      (local.set $1
       (i32.add
        (local.get $1)
        (i32.const 4)
       )
      )
      (br_if $label$7
       (i32.lt_u
        (local.tee $5
         (i32.add
          (local.get $5)
          (i32.const 4)
         )
        )
        (local.get $3)
       )
      )
      (br $label$5)
     )
    )
    (br_if $label$5
     (i32.lt_s
      (local.get $8)
      (i32.const 1)
     )
    )
    (local.set $1
     (local.get $9)
    )
    (loop $label$8
     (i32.store
      (local.get $5)
      (i32.load
       (local.get $1)
      )
     )
     (local.set $1
      (i32.add
       (local.get $1)
       (i32.const 4)
      )
     )
     (br_if $label$8
      (i32.lt_u
       (local.tee $5
        (i32.add
         (local.get $5)
         (i32.const 4)
        )
       )
       (local.get $3)
      )
     )
    )
   )
   (local.set $2
    (i32.and
     (local.get $7)
     (i32.const 3)
    )
   )
   (local.set $1
    (i32.add
     (local.get $9)
     (local.get $8)
    )
   )
  )
  (block $label$9
   (br_if $label$9
    (i32.eqz
     (local.get $2)
    )
   )
   (local.set $5
    (i32.add
     (local.get $3)
     (local.get $2)
    )
   )
   (loop $label$10
    (i32.store8
     (local.get $3)
     (i32.load8_u
      (local.get $1)
     )
    )
    (local.set $1
     (i32.add
      (local.get $1)
      (i32.const 1)
     )
    )
    (br_if $label$10
     (i32.lt_u
      (local.tee $3
       (i32.add
        (local.get $3)
        (i32.const 1)
       )
      )
      (local.get $5)
     )
    )
   )
  )
  (local.get $0)
 )
 ;; custom section ".debug_info", size 405121
 ;; custom section ".debug_pubtypes", size 324
 ;; custom section ".debug_ranges", size 172968
 ;; custom section ".debug_abbrev", size 3967
 ;; custom section "__wasm_bindgen_unstable", size 248
 ;; custom section ".debug_line", size 275986
 ;; custom section ".debug_str", size 626907
 ;; custom section ".debug_pubnames", size 228492
 ;; custom section "producers", size 75
)
