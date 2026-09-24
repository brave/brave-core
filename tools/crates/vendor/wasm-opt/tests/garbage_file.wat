(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func (param i32 i32)))
  (type (;2;) (func (param i32) (result i32)))
  (type (;3;) (func))
  (type (;4;) (func (param i32 i32) (result i32)))
  (type (;5;) (func (param i32 i32 i32)))
  (type (;6;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;7;) (func (param i32 i32 i32) (result i32)))
  (type (;8;) (func (result i32)))
  (type (;9;) (func (param i32 i32 i32 i32 i32) (result i32)))
  (import "__wbindgen_placeholder__" "__wbindgen_describe" (func $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E (type 0)))
  (import "__wbindgen_placeholder__" "__wbg_alert_f30b78c50df83b2d" (func $_ZN11hello_world5alert28__wbg_alert_f30b78c50df83b2d17he1e4a4b8d8d0ea44E (type 1)))
  (import "__wbindgen_externref_xform__" "__wbindgen_externref_table_grow" (func $_ZN12wasm_bindgen9externref31__wbindgen_externref_table_grow17ha4f746c989afa1c2E (type 2)))
  (import "__wbindgen_externref_xform__" "__wbindgen_externref_table_set_null" (func $_ZN12wasm_bindgen9externref35__wbindgen_externref_table_set_null17h958e92ab89f726f8E (type 0)))
  (func $__wbindgen_describe___wbg_alert_f30b78c50df83b2d (type 3)
    call $_ZN12wasm_bindgen4__rt19link_mem_intrinsics17hdfca264069c2a983E
    i32.const 11
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    i32.const 0
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    i32.const 1
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    i32.const 15
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    call $_ZN60_$LT$str$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h75ce916392c06b84E
    call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E
    call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E)
  (func $greet (type 3)
    i32.const 1048576
    i32.const 13
    call $_ZN11hello_world5alert28__wbg_alert_f30b78c50df83b2d17he1e4a4b8d8d0ea44E)
  (func $__wbindgen_describe_greet (type 3)
    call $_ZN12wasm_bindgen4__rt19link_mem_intrinsics17hdfca264069c2a983E
    i32.const 11
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    i32.const 0
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    i32.const 0
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E
    call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E
    call $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E)
  (func $__rust_alloc (type 4) (param i32 i32) (result i32)
    (local i32)
    local.get 0
    local.get 1
    call $__rdl_alloc
    local.set 2
    local.get 2
    return)
  (func $__rust_dealloc (type 5) (param i32 i32 i32)
    local.get 0
    local.get 1
    local.get 2
    call $__rdl_dealloc
    return)
  (func $__rust_realloc (type 6) (param i32 i32 i32 i32) (result i32)
    (local i32)
    local.get 0
    local.get 1
    local.get 2
    local.get 3
    call $__rdl_realloc
    local.set 4
    local.get 4
    return)
  (func $_ZN12wasm_bindgen4__rt19link_mem_intrinsics17hdfca264069c2a983E (type 3)
    call $_ZN12wasm_bindgen9externref15link_intrinsics17h746d56ace27b8dbdE)
  (func $__wbindgen_exn_store (type 0) (param i32)
    i32.const 0
    local.get 0
    i32.store offset=1048596
    i32.const 0
    i32.const 1
    i32.store8 offset=1048592)
  (func $__wbindgen_malloc (type 2) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      i32.const 2147483644
      i32.gt_u
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 0
        br_if 0 (;@2;)
        i32.const 4
        return
      end
      local.get 0
      local.get 0
      i32.const 2147483645
      i32.lt_u
      i32.const 2
      i32.shl
      call $__rust_alloc
      local.tee 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      return
    end
    call $_ZN12wasm_bindgen4__rt14malloc_failure17hd9616718b81cbbb5E
    unreachable)
  (func $_ZN12wasm_bindgen4__rt14malloc_failure17hd9616718b81cbbb5E (type 3)
    call $_ZN3std7process5abort17hf7c8bef35d3938e7E
    unreachable)
  (func $__wbindgen_realloc (type 7) (param i32 i32 i32) (result i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 2147483644
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        i32.const 4
        local.get 2
        call $__rust_realloc
        local.tee 1
        br_if 1 (;@1;)
      end
      call $_ZN12wasm_bindgen4__rt14malloc_failure17hd9616718b81cbbb5E
      unreachable
    end
    local.get 1)
  (func $__wbindgen_free (type 1) (param i32 i32)
    block  ;; label = @1
      local.get 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      local.get 1
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN65_$LT$$LP$$RP$$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h770faab4f3cdf7a3E (type 3)
    i32.const 26
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E)
  (func $_ZN60_$LT$str$u20$as$u20$wasm_bindgen..describe..WasmDescribe$GT$8describe17h75ce916392c06b84E (type 3)
    i32.const 14
    call $_ZN12wasm_bindgen19__wbindgen_describe17h343cf312c5d90ee6E)
  (func $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE (type 3)
    call $_ZN3std7process5abort17hf7c8bef35d3938e7E
    unreachable)
  (func $__externref_table_alloc (type 8) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=1048600
        i32.eqz
        br_if 0 (;@2;)
        i32.const 0
        i32.load offset=1048604
        local.set 0
        br 1 (;@1;)
      end
      i32.const 0
      i64.const 0
      i64.store offset=1048616 align=4
      i32.const 0
      i64.const 0
      i64.store offset=1048608 align=4
      i32.const 0
      i32.const 1
      i32.store offset=1048600
      i32.const 4
      local.set 0
    end
    i32.const 0
    i32.const 4
    i32.store offset=1048604
    i32.const 0
    i32.load offset=1048608
    local.set 1
    i32.const 0
    i32.load offset=1048612
    local.set 2
    i32.const 0
    i64.const 0
    i64.store offset=1048608 align=4
    i32.const 0
    i32.load offset=1048620
    local.set 3
    i32.const 0
    i32.load offset=1048616
    local.set 4
    i32.const 0
    i64.const 0
    i64.store offset=1048616 align=4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          local.get 2
          i32.eq
          br_if 0 (;@3;)
          local.get 2
          local.set 2
          local.get 1
          local.set 5
          local.get 0
          local.set 6
          br 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            local.get 1
            i32.eq
            br_if 0 (;@4;)
            local.get 1
            local.set 5
            local.get 0
            local.set 6
            br 1 (;@3;)
          end
          local.get 1
          i32.const 128
          local.get 1
          i32.const 128
          i32.gt_u
          select
          local.tee 5
          call $_ZN12wasm_bindgen9externref31__wbindgen_externref_table_grow17ha4f746c989afa1c2E
          local.tee 6
          i32.const -1
          i32.eq
          br_if 2 (;@1;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              br_if 0 (;@5;)
              local.get 6
              local.set 3
              br 1 (;@4;)
            end
            local.get 3
            local.get 1
            i32.add
            local.get 6
            i32.ne
            br_if 3 (;@1;)
          end
          local.get 5
          local.get 1
          i32.add
          local.tee 5
          i32.const 2
          i32.shl
          local.tee 6
          i32.const 2147483644
          i32.gt_u
          br_if 2 (;@1;)
          local.get 6
          i32.const 4
          call $__rust_alloc
          local.tee 6
          i32.eqz
          br_if 2 (;@1;)
          local.get 6
          local.get 0
          local.get 1
          i32.const 2
          i32.shl
          local.tee 7
          call $memcpy
          drop
          local.get 1
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.const 1073741823
          i32.and
          local.get 1
          i32.ne
          br_if 0 (;@3;)
          local.get 7
          i32.const -2147483645
          i32.add
          i32.const -2147483644
          i32.lt_u
          br_if 0 (;@3;)
          local.get 0
          local.get 7
          i32.const 4
          call $__rust_dealloc
        end
        local.get 2
        local.get 5
        i32.ge_u
        br_if 1 (;@1;)
        local.get 6
        local.get 2
        i32.const 2
        i32.shl
        i32.add
        local.get 2
        i32.const 1
        i32.add
        local.tee 2
        i32.store
      end
      local.get 4
      local.get 2
      i32.ge_u
      br_if 0 (;@1;)
      local.get 6
      i32.eqz
      br_if 0 (;@1;)
      local.get 6
      local.get 4
      i32.const 2
      i32.shl
      i32.add
      i32.load
      local.set 1
      i32.const 0
      local.get 3
      i32.store offset=1048620
      i32.const 0
      local.get 1
      i32.store offset=1048616
      i32.const 0
      local.get 2
      i32.store offset=1048612
      i32.const 0
      i32.load offset=1048608
      local.set 2
      i32.const 0
      local.get 5
      i32.store offset=1048608
      i32.const 0
      i32.load offset=1048604
      local.set 1
      i32.const 0
      local.get 6
      i32.store offset=1048604
      block  ;; label = @2
        local.get 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.const 1073741823
        i32.and
        local.get 2
        i32.ne
        br_if 0 (;@2;)
        local.get 2
        i32.const 2
        i32.shl
        local.tee 6
        i32.const -2147483645
        i32.add
        i32.const -2147483644
        i32.lt_u
        br_if 0 (;@2;)
        local.get 1
        local.get 6
        i32.const 4
        call $__rust_dealloc
      end
      local.get 3
      local.get 4
      i32.add
      return
    end
    call $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE
    unreachable)
  (func $__externref_table_dealloc (type 0) (param i32)
    (local i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const 36
        i32.lt_u
        br_if 0 (;@2;)
        local.get 0
        call $_ZN12wasm_bindgen9externref35__wbindgen_externref_table_set_null17h958e92ab89f726f8E
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1048600
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=1048604
            local.set 1
            br 1 (;@3;)
          end
          i32.const 0
          i64.const 0
          i64.store offset=1048616 align=4
          i32.const 0
          i64.const 0
          i64.store offset=1048608 align=4
          i32.const 0
          i32.const 1
          i32.store offset=1048600
          i32.const 4
          local.set 1
        end
        i32.const 0
        i32.const 4
        i32.store offset=1048604
        i32.const 0
        i32.load offset=1048612
        local.set 2
        i32.const 0
        i32.load offset=1048608
        local.set 3
        i32.const 0
        i64.const 0
        i64.store offset=1048608 align=4
        i32.const 0
        i32.load offset=1048616
        local.set 4
        i32.const 0
        i32.load offset=1048620
        local.set 5
        i32.const 0
        i64.const 0
        i64.store offset=1048616 align=4
        local.get 5
        local.get 0
        i32.gt_u
        br_if 1 (;@1;)
        local.get 0
        local.get 5
        i32.sub
        local.tee 0
        local.get 2
        i32.ge_u
        br_if 1 (;@1;)
        local.get 1
        i32.eqz
        br_if 1 (;@1;)
        local.get 1
        local.get 0
        i32.const 2
        i32.shl
        i32.add
        local.get 4
        i32.store
        i32.const 0
        local.get 5
        i32.store offset=1048620
        i32.const 0
        local.get 0
        i32.store offset=1048616
        i32.const 0
        local.get 2
        i32.store offset=1048612
        i32.const 0
        i32.load offset=1048608
        local.set 0
        i32.const 0
        local.get 3
        i32.store offset=1048608
        i32.const 0
        i32.load offset=1048604
        local.set 5
        i32.const 0
        local.get 1
        i32.store offset=1048604
        local.get 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        i32.const 1073741823
        i32.and
        local.get 0
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        i32.const 2
        i32.shl
        local.tee 0
        i32.const -2147483645
        i32.add
        i32.const -2147483644
        i32.lt_u
        br_if 0 (;@2;)
        local.get 5
        local.get 0
        i32.const 4
        call $__rust_dealloc
      end
      return
    end
    call $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE
    unreachable)
  (func $__externref_drop_slice (type 1) (param i32 i32)
    block  ;; label = @1
      local.get 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.const 2
      i32.shl
      local.set 1
      loop  ;; label = @2
        local.get 0
        i32.load
        call $__externref_table_dealloc
        local.get 0
        i32.const 4
        i32.add
        local.set 0
        local.get 1
        i32.const -4
        i32.add
        local.tee 1
        br_if 0 (;@2;)
      end
    end)
  (func $__externref_heap_live_count (type 8) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=1048600
        i32.eqz
        br_if 0 (;@2;)
        i32.const 0
        i32.load offset=1048604
        local.set 0
        br 1 (;@1;)
      end
      i32.const 0
      i64.const 0
      i64.store offset=1048616 align=4
      i32.const 0
      i64.const 0
      i64.store offset=1048608 align=4
      i32.const 0
      i32.const 1
      i32.store offset=1048600
      i32.const 4
      local.set 0
    end
    i32.const 0
    i32.const 4
    i32.store offset=1048604
    i32.const 0
    i32.load offset=1048608
    local.set 1
    i32.const 0
    i32.load offset=1048612
    local.set 2
    i32.const 0
    i64.const 0
    i64.store offset=1048608 align=4
    i32.const 0
    i32.load offset=1048620
    local.set 3
    i32.const 0
    i32.load offset=1048616
    local.set 4
    i32.const 0
    i64.const 0
    i64.store offset=1048616 align=4
    i32.const 0
    local.set 5
    block  ;; label = @1
      block  ;; label = @2
        local.get 4
        local.get 2
        i32.ge_u
        br_if 0 (;@2;)
        local.get 0
        i32.eqz
        br_if 1 (;@1;)
        i32.const 0
        local.set 5
        local.get 4
        local.set 6
        loop  ;; label = @3
          local.get 6
          local.get 2
          i32.ge_u
          br_if 2 (;@1;)
          local.get 5
          i32.const 1
          i32.add
          local.set 5
          local.get 0
          local.get 6
          i32.const 2
          i32.shl
          i32.add
          i32.load
          local.tee 6
          local.get 2
          i32.lt_u
          br_if 0 (;@3;)
        end
      end
      i32.const 0
      local.get 3
      i32.store offset=1048620
      i32.const 0
      local.get 4
      i32.store offset=1048616
      i32.const 0
      local.get 2
      i32.store offset=1048612
      i32.const 0
      local.get 0
      i32.store offset=1048604
      i32.const 0
      i32.load offset=1048608
      local.set 6
      i32.const 0
      local.get 1
      i32.store offset=1048608
      block  ;; label = @2
        local.get 6
        i32.eqz
        br_if 0 (;@2;)
        local.get 6
        i32.const 1073741823
        i32.and
        local.get 6
        i32.ne
        br_if 0 (;@2;)
        local.get 6
        i32.const 2
        i32.shl
        local.tee 6
        i32.const -2147483645
        i32.add
        i32.const -2147483644
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 4
        local.get 6
        i32.const 4
        call $__rust_dealloc
      end
      local.get 2
      local.get 5
      i32.sub
      return
    end
    call $_ZN12wasm_bindgen9externref14internal_error17h6e35a70b4a64eecaE
    unreachable)
  (func $_ZN12wasm_bindgen9externref15link_intrinsics17h746d56ace27b8dbdE (type 3))
  (func $_ZN8dlmalloc17Dlmalloc$LT$A$GT$6malloc17he4572c35964f8c9bE (type 4) (param i32 i32) (result i32)
    (local i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.const 9
              i32.lt_u
              br_if 0 (;@5;)
              i32.const 16
              i32.const 8
              call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
              local.get 1
              i32.gt_u
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            local.get 0
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E
            local.set 2
            br 2 (;@2;)
          end
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 1
        end
        call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
        local.set 3
        i32.const 0
        local.set 2
        local.get 3
        local.get 3
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        i32.const 20
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        i32.add
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        i32.add
        i32.sub
        i32.const -65544
        i32.add
        i32.const -9
        i32.and
        i32.const -3
        i32.add
        local.tee 3
        i32.const 0
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        i32.const 2
        i32.shl
        i32.sub
        local.tee 4
        local.get 4
        local.get 3
        i32.gt_u
        select
        local.get 1
        i32.sub
        local.get 0
        i32.le_u
        br_if 0 (;@2;)
        local.get 1
        i32.const 16
        local.get 0
        i32.const 4
        i32.add
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        i32.const -5
        i32.add
        local.get 0
        i32.gt_u
        select
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.tee 4
        i32.add
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        i32.add
        i32.const -4
        i32.add
        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
        local.set 0
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const -1
            i32.add
            local.tee 2
            local.get 3
            i32.and
            br_if 0 (;@4;)
            local.get 0
            local.set 1
            br 1 (;@3;)
          end
          local.get 2
          local.get 3
          i32.add
          i32.const 0
          local.get 1
          i32.sub
          i32.and
          call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
          local.set 2
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 3
          local.get 0
          call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
          local.get 2
          i32.const 0
          local.get 1
          local.get 2
          local.get 0
          i32.sub
          local.get 3
          i32.gt_u
          select
          i32.add
          local.tee 1
          local.get 0
          i32.sub
          local.tee 2
          i32.sub
          local.set 3
          block  ;; label = @4
            local.get 0
            call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
            br_if 0 (;@4;)
            local.get 1
            local.get 3
            call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
            local.get 0
            local.get 2
            call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
            local.get 0
            local.get 2
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
            br 1 (;@3;)
          end
          local.get 0
          i32.load
          local.set 0
          local.get 1
          local.get 3
          i32.store offset=4
          local.get 1
          local.get 0
          local.get 2
          i32.add
          i32.store
        end
        local.get 1
        call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
        br_if 1 (;@1;)
        local.get 1
        call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
        local.tee 0
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.get 4
        i32.add
        i32.le_u
        br_if 1 (;@1;)
        local.get 1
        local.get 4
        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
        local.set 2
        local.get 1
        local.get 4
        call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
        local.get 2
        local.get 0
        local.get 4
        i32.sub
        local.tee 0
        call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
        local.get 2
        local.get 0
        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
        br 1 (;@1;)
      end
      local.get 2
      return
    end
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
    local.set 0
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
    drop
    local.get 0)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E (type 2) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 0
                  i32.const 245
                  i32.lt_u
                  br_if 0 (;@7;)
                  call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
                  local.set 2
                  i32.const 0
                  local.set 3
                  local.get 2
                  local.get 2
                  i32.const 8
                  call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  i32.const 20
                  i32.const 8
                  call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  i32.add
                  i32.const 16
                  i32.const 8
                  call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  i32.add
                  i32.sub
                  i32.const -65544
                  i32.add
                  i32.const -9
                  i32.and
                  i32.const -3
                  i32.add
                  local.tee 2
                  i32.const 0
                  i32.const 16
                  i32.const 8
                  call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  i32.const 2
                  i32.shl
                  i32.sub
                  local.tee 4
                  local.get 4
                  local.get 2
                  i32.gt_u
                  select
                  local.get 0
                  i32.le_u
                  br_if 6 (;@1;)
                  local.get 0
                  i32.const 4
                  i32.add
                  i32.const 8
                  call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                  local.set 2
                  i32.const 0
                  i32.load offset=1048628
                  i32.eqz
                  br_if 5 (;@2;)
                  i32.const 0
                  local.set 5
                  block  ;; label = @8
                    local.get 2
                    i32.const 256
                    i32.lt_u
                    br_if 0 (;@8;)
                    i32.const 31
                    local.set 5
                    local.get 2
                    i32.const 16777215
                    i32.gt_u
                    br_if 0 (;@8;)
                    local.get 2
                    i32.const 6
                    local.get 2
                    i32.const 8
                    i32.shr_u
                    i32.clz
                    local.tee 0
                    i32.sub
                    i32.shr_u
                    i32.const 1
                    i32.and
                    local.get 0
                    i32.const 1
                    i32.shl
                    i32.sub
                    i32.const 62
                    i32.add
                    local.set 5
                  end
                  i32.const 0
                  local.get 2
                  i32.sub
                  local.set 3
                  local.get 5
                  i32.const 2
                  i32.shl
                  i32.const 1048896
                  i32.add
                  i32.load
                  local.tee 4
                  br_if 1 (;@6;)
                  i32.const 0
                  local.set 0
                  i32.const 0
                  local.set 6
                  br 2 (;@5;)
                end
                i32.const 16
                local.get 0
                i32.const 4
                i32.add
                i32.const 16
                i32.const 8
                call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                i32.const -5
                i32.add
                local.get 0
                i32.gt_u
                select
                i32.const 8
                call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                local.set 2
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1048624
                              local.tee 6
                              local.get 2
                              i32.const 3
                              i32.shr_u
                              local.tee 3
                              i32.shr_u
                              local.tee 0
                              i32.const 3
                              i32.and
                              br_if 0 (;@13;)
                              local.get 2
                              i32.const 0
                              i32.load offset=1049024
                              i32.le_u
                              br_if 11 (;@2;)
                              local.get 0
                              br_if 1 (;@12;)
                              i32.const 0
                              i32.load offset=1048628
                              local.tee 0
                              i32.eqz
                              br_if 11 (;@2;)
                              local.get 0
                              call $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE
                              i32.ctz
                              i32.const 2
                              i32.shl
                              i32.const 1048896
                              i32.add
                              i32.load
                              local.tee 4
                              call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                              call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                              local.get 2
                              i32.sub
                              local.set 3
                              block  ;; label = @14
                                local.get 4
                                call $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE
                                local.tee 0
                                i32.eqz
                                br_if 0 (;@14;)
                                loop  ;; label = @15
                                  local.get 0
                                  call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                                  call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                                  local.get 2
                                  i32.sub
                                  local.tee 6
                                  local.get 3
                                  local.get 6
                                  local.get 3
                                  i32.lt_u
                                  local.tee 6
                                  select
                                  local.set 3
                                  local.get 0
                                  local.get 4
                                  local.get 6
                                  select
                                  local.set 4
                                  local.get 0
                                  call $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE
                                  local.tee 0
                                  br_if 0 (;@15;)
                                end
                              end
                              local.get 4
                              call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                              local.tee 0
                              local.get 2
                              call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                              local.set 6
                              i32.const 1048624
                              local.get 4
                              call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
                              local.get 3
                              i32.const 16
                              i32.const 8
                              call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                              i32.lt_u
                              br_if 5 (;@8;)
                              local.get 6
                              call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                              local.set 6
                              local.get 0
                              local.get 2
                              call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                              local.get 6
                              local.get 3
                              call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
                              i32.const 0
                              i32.load offset=1049024
                              local.tee 4
                              i32.eqz
                              br_if 4 (;@9;)
                              local.get 4
                              i32.const 3
                              i32.shr_u
                              local.tee 7
                              i32.const 3
                              i32.shl
                              i32.const 1048632
                              i32.add
                              local.set 8
                              i32.const 0
                              i32.load offset=1049032
                              local.set 4
                              i32.const 0
                              i32.load offset=1048624
                              local.tee 5
                              i32.const 1
                              local.get 7
                              i32.shl
                              local.tee 7
                              i32.and
                              i32.eqz
                              br_if 2 (;@11;)
                              local.get 8
                              i32.load offset=8
                              local.set 7
                              br 3 (;@10;)
                            end
                            block  ;; label = @13
                              block  ;; label = @14
                                local.get 0
                                i32.const -1
                                i32.xor
                                i32.const 1
                                i32.and
                                local.get 3
                                i32.add
                                local.tee 2
                                i32.const 3
                                i32.shl
                                local.tee 4
                                i32.const 1048640
                                i32.add
                                i32.load
                                local.tee 0
                                i32.const 8
                                i32.add
                                i32.load
                                local.tee 3
                                local.get 4
                                i32.const 1048632
                                i32.add
                                local.tee 4
                                i32.eq
                                br_if 0 (;@14;)
                                local.get 3
                                local.get 4
                                i32.store offset=12
                                local.get 4
                                local.get 3
                                i32.store offset=8
                                br 1 (;@13;)
                              end
                              i32.const 0
                              local.get 6
                              i32.const -2
                              local.get 2
                              i32.rotl
                              i32.and
                              i32.store offset=1048624
                            end
                            local.get 0
                            local.get 2
                            i32.const 3
                            i32.shl
                            call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
                            local.get 0
                            call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                            local.set 3
                            br 11 (;@1;)
                          end
                          block  ;; label = @12
                            block  ;; label = @13
                              i32.const 1
                              local.get 3
                              i32.const 31
                              i32.and
                              local.tee 3
                              i32.shl
                              call $_ZN8dlmalloc8dlmalloc9left_bits17hd43e75bebd2d32bdE
                              local.get 0
                              local.get 3
                              i32.shl
                              i32.and
                              call $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE
                              i32.ctz
                              local.tee 3
                              i32.const 3
                              i32.shl
                              local.tee 6
                              i32.const 1048640
                              i32.add
                              i32.load
                              local.tee 0
                              i32.const 8
                              i32.add
                              i32.load
                              local.tee 4
                              local.get 6
                              i32.const 1048632
                              i32.add
                              local.tee 6
                              i32.eq
                              br_if 0 (;@13;)
                              local.get 4
                              local.get 6
                              i32.store offset=12
                              local.get 6
                              local.get 4
                              i32.store offset=8
                              br 1 (;@12;)
                            end
                            i32.const 0
                            i32.const 0
                            i32.load offset=1048624
                            i32.const -2
                            local.get 3
                            i32.rotl
                            i32.and
                            i32.store offset=1048624
                          end
                          local.get 0
                          local.get 2
                          call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                          local.get 0
                          local.get 2
                          call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                          local.tee 4
                          local.get 3
                          i32.const 3
                          i32.shl
                          local.get 2
                          i32.sub
                          local.tee 6
                          call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1049024
                            local.tee 2
                            i32.eqz
                            br_if 0 (;@12;)
                            local.get 2
                            i32.const 3
                            i32.shr_u
                            local.tee 8
                            i32.const 3
                            i32.shl
                            i32.const 1048632
                            i32.add
                            local.set 3
                            i32.const 0
                            i32.load offset=1049032
                            local.set 2
                            block  ;; label = @13
                              block  ;; label = @14
                                i32.const 0
                                i32.load offset=1048624
                                local.tee 7
                                i32.const 1
                                local.get 8
                                i32.shl
                                local.tee 8
                                i32.and
                                i32.eqz
                                br_if 0 (;@14;)
                                local.get 3
                                i32.load offset=8
                                local.set 8
                                br 1 (;@13;)
                              end
                              i32.const 0
                              local.get 7
                              local.get 8
                              i32.or
                              i32.store offset=1048624
                              local.get 3
                              local.set 8
                            end
                            local.get 3
                            local.get 2
                            i32.store offset=8
                            local.get 8
                            local.get 2
                            i32.store offset=12
                            local.get 2
                            local.get 3
                            i32.store offset=12
                            local.get 2
                            local.get 8
                            i32.store offset=8
                          end
                          i32.const 0
                          local.get 4
                          i32.store offset=1049032
                          i32.const 0
                          local.get 6
                          i32.store offset=1049024
                          local.get 0
                          call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                          local.set 3
                          br 10 (;@1;)
                        end
                        i32.const 0
                        local.get 5
                        local.get 7
                        i32.or
                        i32.store offset=1048624
                        local.get 8
                        local.set 7
                      end
                      local.get 8
                      local.get 4
                      i32.store offset=8
                      local.get 7
                      local.get 4
                      i32.store offset=12
                      local.get 4
                      local.get 8
                      i32.store offset=12
                      local.get 4
                      local.get 7
                      i32.store offset=8
                    end
                    i32.const 0
                    local.get 6
                    i32.store offset=1049032
                    i32.const 0
                    local.get 3
                    i32.store offset=1049024
                    br 1 (;@7;)
                  end
                  local.get 0
                  local.get 3
                  local.get 2
                  i32.add
                  call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
                end
                local.get 0
                call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                local.tee 3
                br_if 5 (;@1;)
                br 4 (;@2;)
              end
              local.get 2
              local.get 5
              call $_ZN8dlmalloc8dlmalloc24leftshift_for_tree_index17hd789c537cab28411E
              i32.shl
              local.set 8
              i32.const 0
              local.set 0
              i32.const 0
              local.set 6
              loop  ;; label = @6
                block  ;; label = @7
                  local.get 4
                  call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
                  call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                  local.tee 7
                  local.get 2
                  i32.lt_u
                  br_if 0 (;@7;)
                  local.get 7
                  local.get 2
                  i32.sub
                  local.tee 7
                  local.get 3
                  i32.ge_u
                  br_if 0 (;@7;)
                  local.get 7
                  local.set 3
                  local.get 4
                  local.set 6
                  local.get 7
                  br_if 0 (;@7;)
                  i32.const 0
                  local.set 3
                  local.get 4
                  local.set 6
                  local.get 4
                  local.set 0
                  br 3 (;@4;)
                end
                local.get 4
                i32.const 20
                i32.add
                i32.load
                local.tee 7
                local.get 0
                local.get 7
                local.get 4
                local.get 8
                i32.const 29
                i32.shr_u
                i32.const 4
                i32.and
                i32.add
                i32.const 16
                i32.add
                i32.load
                local.tee 4
                i32.ne
                select
                local.get 0
                local.get 7
                select
                local.set 0
                local.get 8
                i32.const 1
                i32.shl
                local.set 8
                local.get 4
                br_if 0 (;@6;)
              end
            end
            block  ;; label = @5
              local.get 0
              local.get 6
              i32.or
              br_if 0 (;@5;)
              i32.const 0
              local.set 6
              i32.const 1
              local.get 5
              i32.shl
              call $_ZN8dlmalloc8dlmalloc9left_bits17hd43e75bebd2d32bdE
              i32.const 0
              i32.load offset=1048628
              i32.and
              local.tee 0
              i32.eqz
              br_if 3 (;@2;)
              local.get 0
              call $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE
              i32.ctz
              i32.const 2
              i32.shl
              i32.const 1048896
              i32.add
              i32.load
              local.set 0
            end
            local.get 0
            i32.eqz
            br_if 1 (;@3;)
          end
          loop  ;; label = @4
            local.get 0
            local.get 6
            local.get 0
            call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
            call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
            local.tee 4
            local.get 2
            i32.ge_u
            local.get 4
            local.get 2
            i32.sub
            local.tee 4
            local.get 3
            i32.lt_u
            i32.and
            local.tee 8
            select
            local.set 6
            local.get 4
            local.get 3
            local.get 8
            select
            local.set 3
            local.get 0
            call $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE
            local.tee 0
            br_if 0 (;@4;)
          end
        end
        local.get 6
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          i32.const 0
          i32.load offset=1049024
          local.tee 0
          local.get 2
          i32.lt_u
          br_if 0 (;@3;)
          local.get 3
          local.get 0
          local.get 2
          i32.sub
          i32.ge_u
          br_if 1 (;@2;)
        end
        local.get 6
        call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
        local.tee 0
        local.get 2
        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
        local.set 4
        i32.const 1048624
        local.get 6
        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.const 16
            i32.const 8
            call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            i32.lt_u
            br_if 0 (;@4;)
            local.get 0
            local.get 2
            call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
            local.get 4
            local.get 3
            call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
            block  ;; label = @5
              local.get 3
              i32.const 256
              i32.lt_u
              br_if 0 (;@5;)
              i32.const 1048624
              local.get 4
              local.get 3
              call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
              br 2 (;@3;)
            end
            local.get 3
            i32.const 3
            i32.shr_u
            local.tee 6
            i32.const 3
            i32.shl
            i32.const 1048632
            i32.add
            local.set 3
            block  ;; label = @5
              block  ;; label = @6
                i32.const 0
                i32.load offset=1048624
                local.tee 8
                i32.const 1
                local.get 6
                i32.shl
                local.tee 6
                i32.and
                i32.eqz
                br_if 0 (;@6;)
                local.get 3
                i32.load offset=8
                local.set 6
                br 1 (;@5;)
              end
              i32.const 0
              local.get 8
              local.get 6
              i32.or
              i32.store offset=1048624
              local.get 3
              local.set 6
            end
            local.get 3
            local.get 4
            i32.store offset=8
            local.get 6
            local.get 4
            i32.store offset=12
            local.get 4
            local.get 3
            i32.store offset=12
            local.get 4
            local.get 6
            i32.store offset=8
            br 1 (;@3;)
          end
          local.get 0
          local.get 3
          local.get 2
          i32.add
          call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
        end
        local.get 0
        call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
        local.tee 3
        br_if 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        i32.const 0
                        i32.load offset=1049024
                        local.tee 3
                        local.get 2
                        i32.ge_u
                        br_if 0 (;@10;)
                        i32.const 0
                        i32.load offset=1049028
                        local.tee 0
                        local.get 2
                        i32.gt_u
                        br_if 2 (;@8;)
                        local.get 1
                        i32.const 1048624
                        local.get 2
                        call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
                        local.tee 0
                        i32.sub
                        local.get 0
                        i32.const 8
                        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                        i32.add
                        i32.const 20
                        i32.const 8
                        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                        i32.add
                        i32.const 16
                        i32.const 8
                        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                        i32.add
                        i32.const 8
                        i32.add
                        i32.const 65536
                        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                        call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5alloc17he1272ca423b0b1b4E
                        local.get 1
                        i32.load
                        local.tee 3
                        br_if 1 (;@9;)
                        i32.const 0
                        local.set 3
                        br 9 (;@1;)
                      end
                      i32.const 0
                      i32.load offset=1049032
                      local.set 0
                      block  ;; label = @10
                        local.get 3
                        local.get 2
                        i32.sub
                        local.tee 3
                        i32.const 16
                        i32.const 8
                        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                        i32.ge_u
                        br_if 0 (;@10;)
                        i32.const 0
                        i32.const 0
                        i32.store offset=1049032
                        i32.const 0
                        i32.load offset=1049024
                        local.set 2
                        i32.const 0
                        i32.const 0
                        i32.store offset=1049024
                        local.get 0
                        local.get 2
                        call $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E
                        local.get 0
                        call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                        local.set 3
                        br 9 (;@1;)
                      end
                      local.get 0
                      local.get 2
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      local.set 4
                      i32.const 0
                      local.get 3
                      i32.store offset=1049024
                      i32.const 0
                      local.get 4
                      i32.store offset=1049032
                      local.get 4
                      local.get 3
                      call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
                      local.get 0
                      local.get 2
                      call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                      local.set 3
                      br 8 (;@1;)
                    end
                    local.get 1
                    i32.load offset=8
                    local.set 5
                    i32.const 0
                    i32.const 0
                    i32.load offset=1049040
                    local.get 1
                    i32.load offset=4
                    local.tee 8
                    i32.add
                    local.tee 0
                    i32.store offset=1049040
                    i32.const 0
                    i32.const 0
                    i32.load offset=1049044
                    local.tee 4
                    local.get 0
                    local.get 4
                    local.get 0
                    i32.gt_u
                    select
                    i32.store offset=1049044
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1049036
                          i32.eqz
                          br_if 0 (;@11;)
                          i32.const 1049048
                          local.set 0
                          loop  ;; label = @12
                            local.get 3
                            local.get 0
                            call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                            i32.eq
                            br_if 2 (;@10;)
                            local.get 0
                            i32.load offset=8
                            local.tee 0
                            br_if 0 (;@12;)
                            br 3 (;@9;)
                          end
                        end
                        i32.const 0
                        i32.load offset=1049068
                        local.tee 0
                        i32.eqz
                        br_if 3 (;@7;)
                        local.get 3
                        local.get 0
                        i32.lt_u
                        br_if 3 (;@7;)
                        br 7 (;@3;)
                      end
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
                      br_if 0 (;@9;)
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc7Segment9sys_flags17h6d168430a1d92f9aE
                      local.get 5
                      i32.ne
                      br_if 0 (;@9;)
                      local.get 0
                      i32.const 0
                      i32.load offset=1049036
                      call $_ZN8dlmalloc8dlmalloc7Segment5holds17h276a4b63e2947208E
                      br_if 3 (;@6;)
                    end
                    i32.const 0
                    i32.const 0
                    i32.load offset=1049068
                    local.tee 0
                    local.get 3
                    local.get 3
                    local.get 0
                    i32.gt_u
                    select
                    i32.store offset=1049068
                    local.get 3
                    local.get 8
                    i32.add
                    local.set 4
                    i32.const 1049048
                    local.set 0
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          loop  ;; label = @12
                            local.get 0
                            i32.load
                            local.get 4
                            i32.eq
                            br_if 1 (;@11;)
                            local.get 0
                            i32.load offset=8
                            local.tee 0
                            br_if 0 (;@12;)
                            br 2 (;@10;)
                          end
                        end
                        local.get 0
                        call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
                        br_if 0 (;@10;)
                        local.get 0
                        call $_ZN8dlmalloc8dlmalloc7Segment9sys_flags17h6d168430a1d92f9aE
                        local.get 5
                        i32.eq
                        br_if 1 (;@9;)
                      end
                      i32.const 0
                      i32.load offset=1049036
                      local.set 4
                      i32.const 1049048
                      local.set 0
                      block  ;; label = @10
                        loop  ;; label = @11
                          block  ;; label = @12
                            local.get 0
                            i32.load
                            local.get 4
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 0
                            call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                            local.get 4
                            i32.gt_u
                            br_if 2 (;@10;)
                          end
                          local.get 0
                          i32.load offset=8
                          local.tee 0
                          br_if 0 (;@11;)
                        end
                        i32.const 0
                        local.set 0
                      end
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                      local.tee 6
                      i32.const 20
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.tee 9
                      i32.sub
                      i32.const -23
                      i32.add
                      local.set 0
                      local.get 4
                      local.get 0
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                      local.tee 7
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.get 7
                      i32.sub
                      i32.add
                      local.tee 0
                      local.get 0
                      local.get 4
                      i32.const 16
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      i32.add
                      i32.lt_u
                      select
                      local.tee 7
                      call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                      local.set 10
                      local.get 7
                      local.get 9
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      local.set 0
                      call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
                      local.tee 11
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.set 12
                      i32.const 20
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.set 13
                      i32.const 16
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.set 14
                      i32.const 0
                      local.get 3
                      local.get 3
                      call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                      local.tee 15
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.get 15
                      i32.sub
                      local.tee 16
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      local.tee 15
                      i32.store offset=1049036
                      i32.const 0
                      local.get 11
                      local.get 8
                      i32.add
                      local.get 14
                      local.get 12
                      local.get 13
                      i32.add
                      i32.add
                      local.get 16
                      i32.add
                      i32.sub
                      local.tee 11
                      i32.store offset=1049028
                      local.get 15
                      local.get 11
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
                      local.tee 12
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.set 13
                      i32.const 20
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.set 14
                      i32.const 16
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      local.set 16
                      local.get 15
                      local.get 11
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      local.set 15
                      i32.const 0
                      i32.const 2097152
                      i32.store offset=1049064
                      local.get 15
                      local.get 16
                      local.get 14
                      local.get 13
                      local.get 12
                      i32.sub
                      i32.add
                      i32.add
                      i32.store offset=4
                      local.get 7
                      local.get 9
                      call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                      i32.const 0
                      i64.load offset=1049048 align=4
                      local.set 17
                      local.get 10
                      i32.const 8
                      i32.add
                      i32.const 0
                      i64.load offset=1049056 align=4
                      i64.store align=4
                      local.get 10
                      local.get 17
                      i64.store align=4
                      i32.const 0
                      local.get 5
                      i32.store offset=1049060
                      i32.const 0
                      local.get 8
                      i32.store offset=1049052
                      i32.const 0
                      local.get 3
                      i32.store offset=1049048
                      i32.const 0
                      local.get 10
                      i32.store offset=1049056
                      loop  ;; label = @10
                        local.get 0
                        i32.const 4
                        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                        local.set 3
                        local.get 0
                        call $_ZN8dlmalloc8dlmalloc5Chunk14fencepost_head17h32cfaa035be31489E
                        i32.store offset=4
                        local.get 3
                        local.set 0
                        local.get 6
                        local.get 3
                        i32.const 4
                        i32.add
                        i32.gt_u
                        br_if 0 (;@10;)
                      end
                      local.get 7
                      local.get 4
                      i32.eq
                      br_if 7 (;@2;)
                      local.get 7
                      local.get 4
                      i32.sub
                      local.set 0
                      local.get 4
                      local.get 0
                      local.get 4
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
                      block  ;; label = @10
                        local.get 0
                        i32.const 256
                        i32.lt_u
                        br_if 0 (;@10;)
                        i32.const 1048624
                        local.get 4
                        local.get 0
                        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
                        br 8 (;@2;)
                      end
                      local.get 0
                      i32.const 3
                      i32.shr_u
                      local.tee 3
                      i32.const 3
                      i32.shl
                      i32.const 1048632
                      i32.add
                      local.set 0
                      block  ;; label = @10
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1048624
                          local.tee 6
                          i32.const 1
                          local.get 3
                          i32.shl
                          local.tee 3
                          i32.and
                          i32.eqz
                          br_if 0 (;@11;)
                          local.get 0
                          i32.load offset=8
                          local.set 3
                          br 1 (;@10;)
                        end
                        i32.const 0
                        local.get 6
                        local.get 3
                        i32.or
                        i32.store offset=1048624
                        local.get 0
                        local.set 3
                      end
                      local.get 0
                      local.get 4
                      i32.store offset=8
                      local.get 3
                      local.get 4
                      i32.store offset=12
                      local.get 4
                      local.get 0
                      i32.store offset=12
                      local.get 4
                      local.get 3
                      i32.store offset=8
                      br 7 (;@2;)
                    end
                    local.get 0
                    i32.load
                    local.set 6
                    local.get 0
                    local.get 3
                    i32.store
                    local.get 0
                    local.get 0
                    i32.load offset=4
                    local.get 8
                    i32.add
                    i32.store offset=4
                    local.get 3
                    call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                    local.tee 0
                    i32.const 8
                    call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                    local.set 4
                    local.get 6
                    call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                    local.tee 8
                    i32.const 8
                    call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                    local.set 7
                    local.get 3
                    local.get 4
                    local.get 0
                    i32.sub
                    i32.add
                    local.tee 3
                    local.get 2
                    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                    local.set 4
                    local.get 3
                    local.get 2
                    call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                    local.get 6
                    local.get 7
                    local.get 8
                    i32.sub
                    i32.add
                    local.tee 0
                    local.get 2
                    local.get 3
                    i32.add
                    i32.sub
                    local.set 2
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1049036
                      local.get 0
                      i32.eq
                      br_if 0 (;@9;)
                      i32.const 0
                      i32.load offset=1049032
                      local.get 0
                      i32.eq
                      br_if 4 (;@5;)
                      local.get 0
                      call $_ZN8dlmalloc8dlmalloc5Chunk5inuse17h4d9d8a6e39f8aee5E
                      br_if 5 (;@4;)
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 0
                          call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                          local.tee 6
                          i32.const 256
                          i32.lt_u
                          br_if 0 (;@11;)
                          i32.const 1048624
                          local.get 0
                          call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
                          br 1 (;@10;)
                        end
                        block  ;; label = @11
                          local.get 0
                          i32.const 12
                          i32.add
                          i32.load
                          local.tee 8
                          local.get 0
                          i32.const 8
                          i32.add
                          i32.load
                          local.tee 7
                          i32.eq
                          br_if 0 (;@11;)
                          local.get 7
                          local.get 8
                          i32.store offset=12
                          local.get 8
                          local.get 7
                          i32.store offset=8
                          br 1 (;@10;)
                        end
                        i32.const 0
                        i32.const 0
                        i32.load offset=1048624
                        i32.const -2
                        local.get 6
                        i32.const 3
                        i32.shr_u
                        i32.rotl
                        i32.and
                        i32.store offset=1048624
                      end
                      local.get 6
                      local.get 2
                      i32.add
                      local.set 2
                      local.get 0
                      local.get 6
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      local.set 0
                      br 5 (;@4;)
                    end
                    i32.const 0
                    local.get 4
                    i32.store offset=1049036
                    i32.const 0
                    i32.const 0
                    i32.load offset=1049028
                    local.get 2
                    i32.add
                    local.tee 0
                    i32.store offset=1049028
                    local.get 4
                    local.get 0
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get 3
                    call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                    local.set 3
                    br 7 (;@1;)
                  end
                  i32.const 0
                  local.get 0
                  local.get 2
                  i32.sub
                  local.tee 3
                  i32.store offset=1049028
                  i32.const 0
                  i32.const 0
                  i32.load offset=1049036
                  local.tee 0
                  local.get 2
                  call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                  local.tee 4
                  i32.store offset=1049036
                  local.get 4
                  local.get 3
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  local.get 0
                  local.get 2
                  call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
                  local.get 0
                  call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
                  local.set 3
                  br 6 (;@1;)
                end
                i32.const 0
                local.get 3
                i32.store offset=1049068
                br 3 (;@3;)
              end
              local.get 0
              local.get 0
              i32.load offset=4
              local.get 8
              i32.add
              i32.store offset=4
              i32.const 1048624
              i32.const 0
              i32.load offset=1049036
              i32.const 0
              i32.load offset=1049028
              local.get 8
              i32.add
              call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$8init_top17h9ba4d179485fee16E
              br 3 (;@2;)
            end
            i32.const 0
            local.get 4
            i32.store offset=1049032
            i32.const 0
            i32.const 0
            i32.load offset=1049024
            local.get 2
            i32.add
            local.tee 0
            i32.store offset=1049024
            local.get 4
            local.get 0
            call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
            local.get 3
            call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
            local.set 3
            br 3 (;@1;)
          end
          local.get 4
          local.get 2
          local.get 0
          call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
          block  ;; label = @4
            local.get 2
            i32.const 256
            i32.lt_u
            br_if 0 (;@4;)
            i32.const 1048624
            local.get 4
            local.get 2
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
            local.get 3
            call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
            local.set 3
            br 3 (;@1;)
          end
          local.get 2
          i32.const 3
          i32.shr_u
          local.tee 2
          i32.const 3
          i32.shl
          i32.const 1048632
          i32.add
          local.set 0
          block  ;; label = @4
            block  ;; label = @5
              i32.const 0
              i32.load offset=1048624
              local.tee 6
              i32.const 1
              local.get 2
              i32.shl
              local.tee 2
              i32.and
              i32.eqz
              br_if 0 (;@5;)
              local.get 0
              i32.load offset=8
              local.set 2
              br 1 (;@4;)
            end
            i32.const 0
            local.get 6
            local.get 2
            i32.or
            i32.store offset=1048624
            local.get 0
            local.set 2
          end
          local.get 0
          local.get 4
          i32.store offset=8
          local.get 2
          local.get 4
          i32.store offset=12
          local.get 4
          local.get 0
          i32.store offset=12
          local.get 4
          local.get 2
          i32.store offset=8
          local.get 3
          call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
          local.set 3
          br 2 (;@1;)
        end
        i32.const 0
        i32.const 4095
        i32.store offset=1049072
        i32.const 0
        local.get 5
        i32.store offset=1049060
        i32.const 0
        local.get 8
        i32.store offset=1049052
        i32.const 0
        local.get 3
        i32.store offset=1049048
        i32.const 0
        i32.const 1048632
        i32.store offset=1048644
        i32.const 0
        i32.const 1048640
        i32.store offset=1048652
        i32.const 0
        i32.const 1048632
        i32.store offset=1048640
        i32.const 0
        i32.const 1048648
        i32.store offset=1048660
        i32.const 0
        i32.const 1048640
        i32.store offset=1048648
        i32.const 0
        i32.const 1048656
        i32.store offset=1048668
        i32.const 0
        i32.const 1048648
        i32.store offset=1048656
        i32.const 0
        i32.const 1048664
        i32.store offset=1048676
        i32.const 0
        i32.const 1048656
        i32.store offset=1048664
        i32.const 0
        i32.const 1048672
        i32.store offset=1048684
        i32.const 0
        i32.const 1048664
        i32.store offset=1048672
        i32.const 0
        i32.const 1048680
        i32.store offset=1048692
        i32.const 0
        i32.const 1048672
        i32.store offset=1048680
        i32.const 0
        i32.const 1048688
        i32.store offset=1048700
        i32.const 0
        i32.const 1048680
        i32.store offset=1048688
        i32.const 0
        i32.const 1048696
        i32.store offset=1048708
        i32.const 0
        i32.const 1048688
        i32.store offset=1048696
        i32.const 0
        i32.const 1048696
        i32.store offset=1048704
        i32.const 0
        i32.const 1048704
        i32.store offset=1048716
        i32.const 0
        i32.const 1048704
        i32.store offset=1048712
        i32.const 0
        i32.const 1048712
        i32.store offset=1048724
        i32.const 0
        i32.const 1048712
        i32.store offset=1048720
        i32.const 0
        i32.const 1048720
        i32.store offset=1048732
        i32.const 0
        i32.const 1048720
        i32.store offset=1048728
        i32.const 0
        i32.const 1048728
        i32.store offset=1048740
        i32.const 0
        i32.const 1048728
        i32.store offset=1048736
        i32.const 0
        i32.const 1048736
        i32.store offset=1048748
        i32.const 0
        i32.const 1048736
        i32.store offset=1048744
        i32.const 0
        i32.const 1048744
        i32.store offset=1048756
        i32.const 0
        i32.const 1048744
        i32.store offset=1048752
        i32.const 0
        i32.const 1048752
        i32.store offset=1048764
        i32.const 0
        i32.const 1048752
        i32.store offset=1048760
        i32.const 0
        i32.const 1048760
        i32.store offset=1048772
        i32.const 0
        i32.const 1048768
        i32.store offset=1048780
        i32.const 0
        i32.const 1048760
        i32.store offset=1048768
        i32.const 0
        i32.const 1048776
        i32.store offset=1048788
        i32.const 0
        i32.const 1048768
        i32.store offset=1048776
        i32.const 0
        i32.const 1048784
        i32.store offset=1048796
        i32.const 0
        i32.const 1048776
        i32.store offset=1048784
        i32.const 0
        i32.const 1048792
        i32.store offset=1048804
        i32.const 0
        i32.const 1048784
        i32.store offset=1048792
        i32.const 0
        i32.const 1048800
        i32.store offset=1048812
        i32.const 0
        i32.const 1048792
        i32.store offset=1048800
        i32.const 0
        i32.const 1048808
        i32.store offset=1048820
        i32.const 0
        i32.const 1048800
        i32.store offset=1048808
        i32.const 0
        i32.const 1048816
        i32.store offset=1048828
        i32.const 0
        i32.const 1048808
        i32.store offset=1048816
        i32.const 0
        i32.const 1048824
        i32.store offset=1048836
        i32.const 0
        i32.const 1048816
        i32.store offset=1048824
        i32.const 0
        i32.const 1048832
        i32.store offset=1048844
        i32.const 0
        i32.const 1048824
        i32.store offset=1048832
        i32.const 0
        i32.const 1048840
        i32.store offset=1048852
        i32.const 0
        i32.const 1048832
        i32.store offset=1048840
        i32.const 0
        i32.const 1048848
        i32.store offset=1048860
        i32.const 0
        i32.const 1048840
        i32.store offset=1048848
        i32.const 0
        i32.const 1048856
        i32.store offset=1048868
        i32.const 0
        i32.const 1048848
        i32.store offset=1048856
        i32.const 0
        i32.const 1048864
        i32.store offset=1048876
        i32.const 0
        i32.const 1048856
        i32.store offset=1048864
        i32.const 0
        i32.const 1048872
        i32.store offset=1048884
        i32.const 0
        i32.const 1048864
        i32.store offset=1048872
        i32.const 0
        i32.const 1048880
        i32.store offset=1048892
        i32.const 0
        i32.const 1048872
        i32.store offset=1048880
        i32.const 0
        i32.const 1048880
        i32.store offset=1048888
        call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
        local.tee 4
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.set 6
        i32.const 20
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.set 7
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.set 5
        i32.const 0
        local.get 3
        local.get 3
        call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
        local.tee 0
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.get 0
        i32.sub
        local.tee 10
        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
        local.tee 0
        i32.store offset=1049036
        i32.const 0
        local.get 4
        local.get 8
        i32.add
        local.get 5
        local.get 6
        local.get 7
        i32.add
        i32.add
        local.get 10
        i32.add
        i32.sub
        local.tee 3
        i32.store offset=1049028
        local.get 0
        local.get 3
        i32.const 1
        i32.or
        i32.store offset=4
        call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
        local.tee 4
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.set 6
        i32.const 20
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.set 8
        i32.const 16
        i32.const 8
        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
        local.set 7
        local.get 0
        local.get 3
        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
        local.set 0
        i32.const 0
        i32.const 2097152
        i32.store offset=1049064
        local.get 0
        local.get 7
        local.get 8
        local.get 6
        local.get 4
        i32.sub
        i32.add
        i32.add
        i32.store offset=4
      end
      i32.const 0
      local.set 3
      i32.const 0
      i32.load offset=1049028
      local.tee 0
      local.get 2
      i32.le_u
      br_if 0 (;@1;)
      i32.const 0
      local.get 0
      local.get 2
      i32.sub
      local.tee 3
      i32.store offset=1049028
      i32.const 0
      i32.const 0
      i32.load offset=1049036
      local.tee 0
      local.get 2
      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
      local.tee 4
      i32.store offset=1049036
      local.get 4
      local.get 3
      i32.const 1
      i32.or
      i32.store offset=4
      local.get 0
      local.get 2
      call $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E
      local.get 0
      call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
      local.set 3
    end
    local.get 1
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 3)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E (type 1) (param i32 i32)
    (local i32 i32 i32 i32)
    local.get 0
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          call $_ZN8dlmalloc8dlmalloc5Chunk6pinuse17h89f5f80c1a4cb95aE
          br_if 0 (;@3;)
          local.get 0
          i32.load
          local.set 3
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
              br_if 0 (;@5;)
              local.get 3
              local.get 1
              i32.add
              local.set 1
              local.get 0
              local.get 3
              call $_ZN8dlmalloc8dlmalloc5Chunk12minus_offset17h39dd10694c91288eE
              local.tee 0
              i32.const 0
              i32.load offset=1049032
              i32.ne
              br_if 1 (;@4;)
              local.get 2
              i32.load offset=4
              i32.const 3
              i32.and
              i32.const 3
              i32.ne
              br_if 2 (;@3;)
              i32.const 0
              local.get 1
              i32.store offset=1049024
              local.get 0
              local.get 1
              local.get 2
              call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
              return
            end
            i32.const 1048624
            local.get 0
            local.get 3
            i32.sub
            local.get 3
            local.get 1
            i32.add
            i32.const 16
            i32.add
            local.tee 0
            call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E
            i32.eqz
            br_if 2 (;@2;)
            i32.const 0
            i32.const 0
            i32.load offset=1049040
            local.get 0
            i32.sub
            i32.store offset=1049040
            return
          end
          block  ;; label = @4
            local.get 3
            i32.const 256
            i32.lt_u
            br_if 0 (;@4;)
            i32.const 1048624
            local.get 0
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
            br 1 (;@3;)
          end
          block  ;; label = @4
            local.get 0
            i32.const 12
            i32.add
            i32.load
            local.tee 4
            local.get 0
            i32.const 8
            i32.add
            i32.load
            local.tee 5
            i32.eq
            br_if 0 (;@4;)
            local.get 5
            local.get 4
            i32.store offset=12
            local.get 4
            local.get 5
            i32.store offset=8
            br 1 (;@3;)
          end
          i32.const 0
          i32.const 0
          i32.load offset=1048624
          i32.const -2
          local.get 3
          i32.const 3
          i32.shr_u
          i32.rotl
          i32.and
          i32.store offset=1048624
        end
        block  ;; label = @3
          local.get 2
          call $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          local.get 1
          local.get 2
          call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
          br 2 (;@1;)
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            i32.const 0
            i32.load offset=1049036
            i32.eq
            br_if 0 (;@4;)
            local.get 2
            i32.const 0
            i32.load offset=1049032
            i32.ne
            br_if 1 (;@3;)
            i32.const 0
            local.get 0
            i32.store offset=1049032
            i32.const 0
            i32.const 0
            i32.load offset=1049024
            local.get 1
            i32.add
            local.tee 1
            i32.store offset=1049024
            local.get 0
            local.get 1
            call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
            return
          end
          i32.const 0
          local.get 0
          i32.store offset=1049036
          i32.const 0
          i32.const 0
          i32.load offset=1049028
          local.get 1
          i32.add
          local.tee 1
          i32.store offset=1049028
          local.get 0
          local.get 1
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 0
          i32.const 0
          i32.load offset=1049032
          i32.ne
          br_if 1 (;@2;)
          i32.const 0
          i32.const 0
          i32.store offset=1049024
          i32.const 0
          i32.const 0
          i32.store offset=1049032
          return
        end
        local.get 2
        call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
        local.tee 3
        local.get 1
        i32.add
        local.set 1
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.const 256
            i32.lt_u
            br_if 0 (;@4;)
            i32.const 1048624
            local.get 2
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
            br 1 (;@3;)
          end
          block  ;; label = @4
            local.get 2
            i32.const 12
            i32.add
            i32.load
            local.tee 4
            local.get 2
            i32.const 8
            i32.add
            i32.load
            local.tee 2
            i32.eq
            br_if 0 (;@4;)
            local.get 2
            local.get 4
            i32.store offset=12
            local.get 4
            local.get 2
            i32.store offset=8
            br 1 (;@3;)
          end
          i32.const 0
          i32.const 0
          i32.load offset=1048624
          i32.const -2
          local.get 3
          i32.const 3
          i32.shr_u
          i32.rotl
          i32.and
          i32.store offset=1048624
        end
        local.get 0
        local.get 1
        call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
        local.get 0
        i32.const 0
        i32.load offset=1049032
        i32.ne
        br_if 1 (;@1;)
        i32.const 0
        local.get 1
        i32.store offset=1049024
      end
      return
    end
    block  ;; label = @1
      local.get 1
      i32.const 256
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 1048624
      local.get 0
      local.get 1
      call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
      return
    end
    local.get 1
    i32.const 3
    i32.shr_u
    local.tee 2
    i32.const 3
    i32.shl
    i32.const 1048632
    i32.add
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=1048624
        local.tee 3
        i32.const 1
        local.get 2
        i32.shl
        local.tee 2
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load offset=8
        local.set 2
        br 1 (;@1;)
      end
      i32.const 0
      local.get 3
      local.get 2
      i32.or
      i32.store offset=1048624
      local.get 1
      local.set 2
    end
    local.get 1
    local.get 0
    i32.store offset=8
    local.get 2
    local.get 0
    i32.store offset=12
    local.get 0
    local.get 1
    i32.store offset=12
    local.get 0
    local.get 2
    i32.store offset=8)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E (type 1) (param i32 i32)
    (local i32 i32 i32 i32 i32)
    local.get 1
    i32.load offset=24
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          call $_ZN8dlmalloc8dlmalloc9TreeChunk4next17h656f2e3867c8acf8E
          local.get 1
          i32.ne
          br_if 0 (;@3;)
          local.get 1
          i32.const 20
          i32.const 16
          local.get 1
          i32.const 20
          i32.add
          local.tee 3
          i32.load
          local.tee 4
          select
          i32.add
          i32.load
          local.tee 5
          br_if 1 (;@2;)
          i32.const 0
          local.set 3
          br 2 (;@1;)
        end
        local.get 1
        call $_ZN8dlmalloc8dlmalloc9TreeChunk4prev17h527f673fd8318adbE
        local.tee 5
        local.get 1
        call $_ZN8dlmalloc8dlmalloc9TreeChunk4next17h656f2e3867c8acf8E
        local.tee 3
        call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
        i32.store offset=12
        local.get 3
        local.get 5
        call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
        i32.store offset=8
        br 1 (;@1;)
      end
      local.get 3
      local.get 1
      i32.const 16
      i32.add
      local.get 4
      select
      local.set 4
      loop  ;; label = @2
        local.get 4
        local.set 6
        local.get 5
        local.tee 3
        i32.const 20
        i32.add
        local.tee 5
        local.get 3
        i32.const 16
        i32.add
        local.get 5
        i32.load
        select
        local.tee 4
        i32.load
        local.tee 5
        br_if 0 (;@2;)
      end
      local.get 6
      i32.const 0
      i32.store
    end
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          local.get 1
          i32.load offset=28
          local.tee 4
          i32.const 2
          i32.shl
          i32.add
          i32.const 272
          i32.add
          local.tee 5
          i32.load
          local.get 1
          i32.eq
          br_if 0 (;@3;)
          local.get 2
          i32.const 16
          i32.const 20
          local.get 2
          i32.load offset=16
          local.get 1
          i32.eq
          select
          i32.add
          local.get 3
          i32.store
          local.get 3
          br_if 1 (;@2;)
          br 2 (;@1;)
        end
        local.get 5
        local.get 3
        i32.store
        local.get 3
        br_if 0 (;@2;)
        local.get 0
        local.get 0
        i32.load offset=4
        i32.const -2
        local.get 4
        i32.rotl
        i32.and
        i32.store offset=4
        return
      end
      local.get 3
      local.get 2
      i32.store offset=24
      block  ;; label = @2
        local.get 1
        i32.load offset=16
        local.tee 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 5
        i32.store offset=16
        local.get 5
        local.get 3
        i32.store offset=24
      end
      local.get 1
      i32.const 20
      i32.add
      i32.load
      local.tee 5
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      i32.const 20
      i32.add
      local.get 5
      i32.store
      local.get 5
      local.get 3
      i32.store offset=24
      return
    end)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE (type 5) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    i32.const 0
    local.set 3
    block  ;; label = @1
      local.get 2
      i32.const 256
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 31
      local.set 3
      local.get 2
      i32.const 16777215
      i32.gt_u
      br_if 0 (;@1;)
      local.get 2
      i32.const 6
      local.get 2
      i32.const 8
      i32.shr_u
      i32.clz
      local.tee 3
      i32.sub
      i32.shr_u
      i32.const 1
      i32.and
      local.get 3
      i32.const 1
      i32.shl
      i32.sub
      i32.const 62
      i32.add
      local.set 3
    end
    local.get 1
    i64.const 0
    i64.store offset=16 align=4
    local.get 1
    local.get 3
    i32.store offset=28
    local.get 1
    call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
    local.set 4
    local.get 0
    local.get 3
    i32.const 2
    i32.shl
    i32.add
    i32.const 272
    i32.add
    local.set 5
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.const 4
              i32.add
              local.tee 0
              i32.load
              local.tee 6
              i32.const 1
              local.get 3
              i32.shl
              local.tee 7
              i32.and
              i32.eqz
              br_if 0 (;@5;)
              local.get 5
              i32.load
              local.set 5
              local.get 3
              call $_ZN8dlmalloc8dlmalloc24leftshift_for_tree_index17hd789c537cab28411E
              local.set 3
              local.get 5
              call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
              call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
              local.get 2
              i32.ne
              br_if 1 (;@4;)
              local.get 5
              local.set 3
              br 2 (;@3;)
            end
            local.get 0
            local.get 6
            local.get 7
            i32.or
            i32.store
            local.get 1
            local.get 5
            i32.store offset=24
            local.get 5
            local.get 1
            i32.store
            br 3 (;@1;)
          end
          local.get 2
          local.get 3
          i32.shl
          local.set 0
          loop  ;; label = @4
            local.get 5
            local.get 0
            i32.const 29
            i32.shr_u
            i32.const 4
            i32.and
            i32.add
            i32.const 16
            i32.add
            local.tee 6
            i32.load
            local.tee 3
            i32.eqz
            br_if 2 (;@2;)
            local.get 0
            i32.const 1
            i32.shl
            local.set 0
            local.get 3
            local.set 5
            local.get 3
            call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
            call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
            local.get 2
            i32.ne
            br_if 0 (;@4;)
          end
        end
        local.get 3
        call $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE
        local.tee 3
        i32.load offset=8
        local.tee 0
        local.get 4
        i32.store offset=12
        local.get 3
        local.get 4
        i32.store offset=8
        local.get 4
        local.get 3
        i32.store offset=12
        local.get 4
        local.get 0
        i32.store offset=8
        local.get 1
        i32.const 0
        i32.store offset=24
        return
      end
      local.get 6
      local.get 1
      i32.store
      local.get 1
      local.get 5
      i32.store offset=24
    end
    local.get 4
    local.get 4
    i32.store offset=8
    local.get 4
    local.get 4
    i32.store offset=12)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$23release_unused_segments17h75b413e6a85f4b60E (type 2) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      local.get 0
      i32.const 432
      i32.add
      i32.load
      local.tee 1
      br_if 0 (;@1;)
      local.get 0
      i32.const 4095
      i32.store offset=448
      i32.const 0
      return
    end
    local.get 0
    i32.const 424
    i32.add
    local.set 2
    i32.const 0
    local.set 3
    i32.const 0
    local.set 4
    loop  ;; label = @1
      local.get 1
      local.tee 5
      i32.load offset=8
      local.set 1
      local.get 5
      i32.load offset=4
      local.set 6
      local.get 5
      i32.load
      local.set 7
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          local.get 5
          i32.const 12
          i32.add
          i32.load
          i32.const 1
          i32.shr_u
          call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$16can_release_part17ha9587956c545036fE
          i32.eqz
          br_if 0 (;@3;)
          local.get 5
          call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
          br_if 0 (;@3;)
          local.get 7
          local.get 7
          call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
          local.tee 8
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.get 8
          i32.sub
          i32.add
          local.tee 8
          call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
          local.set 9
          call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
          local.tee 10
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 11
          i32.const 20
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 12
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 13
          local.get 8
          call $_ZN8dlmalloc8dlmalloc5Chunk5inuse17h4d9d8a6e39f8aee5E
          br_if 0 (;@3;)
          local.get 8
          local.get 9
          i32.add
          local.get 7
          local.get 10
          local.get 6
          i32.add
          local.get 11
          local.get 12
          i32.add
          local.get 13
          i32.add
          i32.sub
          i32.add
          i32.lt_u
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.load offset=408
              local.get 8
              i32.eq
              br_if 0 (;@5;)
              local.get 0
              local.get 8
              call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
              br 1 (;@4;)
            end
            local.get 0
            i32.const 0
            i32.store offset=400
            local.get 0
            i32.const 0
            i32.store offset=408
          end
          block  ;; label = @4
            local.get 0
            local.get 7
            local.get 6
            call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E
            br_if 0 (;@4;)
            local.get 0
            local.get 8
            local.get 9
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
            br 1 (;@3;)
          end
          local.get 0
          local.get 0
          i32.load offset=416
          local.get 6
          i32.sub
          i32.store offset=416
          local.get 2
          local.get 1
          i32.store offset=8
          local.get 6
          local.get 3
          i32.add
          local.set 3
          br 1 (;@2;)
        end
        local.get 5
        local.set 2
      end
      local.get 4
      i32.const 1
      i32.add
      local.set 4
      local.get 1
      br_if 0 (;@1;)
    end
    local.get 0
    local.get 4
    i32.const 4095
    local.get 4
    i32.const 4095
    i32.gt_u
    select
    i32.store offset=448
    local.get 3)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E (type 1) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
    local.set 1
    local.get 1
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
    local.tee 2
    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    local.set 3
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          call $_ZN8dlmalloc8dlmalloc5Chunk6pinuse17h89f5f80c1a4cb95aE
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.set 4
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
              br_if 0 (;@5;)
              local.get 4
              local.get 2
              i32.add
              local.set 2
              local.get 1
              local.get 4
              call $_ZN8dlmalloc8dlmalloc5Chunk12minus_offset17h39dd10694c91288eE
              local.tee 1
              local.get 0
              i32.load offset=408
              i32.ne
              br_if 1 (;@4;)
              local.get 3
              i32.load offset=4
              i32.const 3
              i32.and
              i32.const 3
              i32.ne
              br_if 2 (;@3;)
              local.get 0
              local.get 2
              i32.store offset=400
              local.get 1
              local.get 2
              local.get 3
              call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
              return
            end
            local.get 0
            local.get 1
            local.get 4
            i32.sub
            local.get 4
            local.get 2
            i32.add
            i32.const 16
            i32.add
            local.tee 1
            call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E
            i32.eqz
            br_if 2 (;@2;)
            local.get 0
            local.get 0
            i32.load offset=416
            local.get 1
            i32.sub
            i32.store offset=416
            return
          end
          block  ;; label = @4
            local.get 4
            i32.const 256
            i32.lt_u
            br_if 0 (;@4;)
            local.get 0
            local.get 1
            call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
            br 1 (;@3;)
          end
          block  ;; label = @4
            local.get 1
            i32.const 12
            i32.add
            i32.load
            local.tee 5
            local.get 1
            i32.const 8
            i32.add
            i32.load
            local.tee 6
            i32.eq
            br_if 0 (;@4;)
            local.get 6
            local.get 5
            i32.store offset=12
            local.get 5
            local.get 6
            i32.store offset=8
            br 1 (;@3;)
          end
          local.get 0
          local.get 0
          i32.load
          i32.const -2
          local.get 4
          i32.const 3
          i32.shr_u
          i32.rotl
          i32.and
          i32.store
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            call $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            local.get 2
            local.get 3
            call $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E
            br 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 3
                  local.get 0
                  i32.load offset=412
                  i32.eq
                  br_if 0 (;@7;)
                  local.get 3
                  local.get 0
                  i32.load offset=408
                  i32.ne
                  br_if 1 (;@6;)
                  local.get 0
                  local.get 1
                  i32.store offset=408
                  local.get 0
                  local.get 0
                  i32.load offset=400
                  local.get 2
                  i32.add
                  local.tee 2
                  i32.store offset=400
                  local.get 1
                  local.get 2
                  call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
                  return
                end
                local.get 0
                local.get 1
                i32.store offset=412
                local.get 0
                local.get 0
                i32.load offset=404
                local.get 2
                i32.add
                local.tee 2
                i32.store offset=404
                local.get 1
                local.get 2
                i32.const 1
                i32.or
                i32.store offset=4
                local.get 1
                local.get 0
                i32.load offset=408
                i32.eq
                br_if 1 (;@5;)
                br 2 (;@4;)
              end
              local.get 3
              call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
              local.tee 4
              local.get 2
              i32.add
              local.set 2
              block  ;; label = @6
                block  ;; label = @7
                  local.get 4
                  i32.const 256
                  i32.lt_u
                  br_if 0 (;@7;)
                  local.get 0
                  local.get 3
                  call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
                  br 1 (;@6;)
                end
                block  ;; label = @7
                  local.get 3
                  i32.const 12
                  i32.add
                  i32.load
                  local.tee 5
                  local.get 3
                  i32.const 8
                  i32.add
                  i32.load
                  local.tee 3
                  i32.eq
                  br_if 0 (;@7;)
                  local.get 3
                  local.get 5
                  i32.store offset=12
                  local.get 5
                  local.get 3
                  i32.store offset=8
                  br 1 (;@6;)
                end
                local.get 0
                local.get 0
                i32.load
                i32.const -2
                local.get 4
                i32.const 3
                i32.shr_u
                i32.rotl
                i32.and
                i32.store
              end
              local.get 1
              local.get 2
              call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
              local.get 1
              local.get 0
              i32.load offset=408
              i32.ne
              br_if 2 (;@3;)
              local.get 0
              local.get 2
              i32.store offset=400
              br 3 (;@2;)
            end
            local.get 0
            i32.const 0
            i32.store offset=400
            local.get 0
            i32.const 0
            i32.store offset=408
          end
          local.get 0
          i32.const 440
          i32.add
          i32.load
          local.get 2
          i32.ge_u
          br_if 1 (;@2;)
          call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
          local.set 1
          local.get 1
          local.get 1
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.const 20
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.add
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.add
          i32.sub
          i32.const -65544
          i32.add
          i32.const -9
          i32.and
          i32.const -3
          i32.add
          local.tee 1
          i32.const 0
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.const 2
          i32.shl
          i32.sub
          local.tee 2
          local.get 2
          local.get 1
          i32.gt_u
          select
          i32.eqz
          br_if 1 (;@2;)
          local.get 0
          i32.load offset=412
          local.tee 2
          i32.eqz
          br_if 1 (;@2;)
          call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
          local.tee 1
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 3
          i32.const 20
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 5
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 6
          i32.const 0
          local.set 4
          block  ;; label = @4
            local.get 0
            i32.load offset=404
            local.tee 7
            local.get 6
            local.get 5
            local.get 3
            local.get 1
            i32.sub
            i32.add
            i32.add
            local.tee 1
            i32.le_u
            br_if 0 (;@4;)
            local.get 7
            local.get 1
            i32.const -1
            i32.xor
            i32.add
            i32.const -65536
            i32.and
            local.set 5
            local.get 0
            i32.const 424
            i32.add
            local.tee 3
            local.set 1
            block  ;; label = @5
              loop  ;; label = @6
                block  ;; label = @7
                  local.get 1
                  i32.load
                  local.get 2
                  i32.gt_u
                  br_if 0 (;@7;)
                  local.get 1
                  call $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E
                  local.get 2
                  i32.gt_u
                  br_if 2 (;@5;)
                end
                local.get 1
                i32.load offset=8
                local.tee 1
                br_if 0 (;@6;)
              end
              i32.const 0
              local.set 1
            end
            i32.const 0
            local.set 4
            local.get 1
            call $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E
            br_if 0 (;@4;)
            i32.const 0
            local.set 4
            local.get 0
            local.get 1
            i32.const 12
            i32.add
            i32.load
            i32.const 1
            i32.shr_u
            call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$16can_release_part17ha9587956c545036fE
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 4
            local.get 1
            i32.load offset=4
            local.get 5
            i32.lt_u
            br_if 0 (;@4;)
            loop  ;; label = @5
              block  ;; label = @6
                local.get 1
                local.get 3
                call $_ZN8dlmalloc8dlmalloc7Segment5holds17h276a4b63e2947208E
                i32.eqz
                br_if 0 (;@6;)
                i32.const 0
                local.set 4
                br 2 (;@4;)
              end
              local.get 3
              i32.load offset=8
              local.tee 3
              br_if 0 (;@5;)
            end
            i32.const 0
            local.set 4
            local.get 0
            local.get 1
            i32.load
            local.get 1
            i32.load offset=4
            local.tee 2
            local.get 2
            local.get 5
            i32.sub
            call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9free_part17hfe7db7a0188c71a3E
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 4
            local.get 5
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            local.get 1
            i32.load offset=4
            local.get 5
            i32.sub
            i32.store offset=4
            local.get 0
            local.get 0
            i32.load offset=416
            local.get 5
            i32.sub
            i32.store offset=416
            local.get 0
            i32.load offset=404
            local.set 2
            local.get 0
            i32.load offset=412
            local.set 1
            local.get 0
            local.get 1
            local.get 1
            call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
            local.tee 3
            i32.const 8
            call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            local.get 3
            i32.sub
            local.tee 3
            call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
            local.tee 1
            i32.store offset=412
            local.get 0
            local.get 2
            local.get 5
            local.get 3
            i32.add
            i32.sub
            local.tee 2
            i32.store offset=404
            local.get 1
            local.get 2
            i32.const 1
            i32.or
            i32.store offset=4
            call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
            local.tee 3
            i32.const 8
            call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            local.set 4
            i32.const 20
            i32.const 8
            call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            local.set 6
            i32.const 16
            i32.const 8
            call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
            local.set 7
            local.get 1
            local.get 2
            call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
            local.set 1
            local.get 0
            i32.const 440
            i32.add
            i32.const 2097152
            i32.store
            local.get 1
            local.get 7
            local.get 6
            local.get 4
            local.get 3
            i32.sub
            i32.add
            i32.add
            i32.store offset=4
            local.get 5
            local.set 4
          end
          local.get 4
          i32.const 0
          local.get 0
          call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$23release_unused_segments17h75b413e6a85f4b60E
          i32.sub
          i32.ne
          br_if 1 (;@2;)
          local.get 0
          i32.load offset=404
          local.get 0
          i32.const 440
          i32.add
          i32.load
          i32.le_u
          br_if 1 (;@2;)
          local.get 0
          i32.const 440
          i32.add
          i32.const -1
          i32.store
          return
        end
        local.get 2
        i32.const 256
        i32.lt_u
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        local.get 2
        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18insert_large_chunk17habc5b0b62eef023bE
        local.get 0
        local.get 0
        i32.load offset=448
        i32.const -1
        i32.add
        local.tee 1
        i32.store offset=448
        local.get 1
        br_if 0 (;@2;)
        local.get 0
        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$23release_unused_segments17h75b413e6a85f4b60E
        drop
        return
      end
      return
    end
    local.get 0
    local.get 2
    i32.const 3
    i32.shr_u
    local.tee 3
    i32.const 3
    i32.shl
    i32.add
    i32.const 8
    i32.add
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 4
        i32.const 1
        local.get 3
        i32.shl
        local.tee 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.load offset=8
        local.set 0
        br 1 (;@1;)
      end
      local.get 0
      local.get 4
      local.get 3
      i32.or
      i32.store
      local.get 2
      local.set 0
    end
    local.get 2
    local.get 1
    i32.store offset=8
    local.get 0
    local.get 1
    i32.store offset=12
    local.get 1
    local.get 2
    i32.store offset=12
    local.get 1
    local.get 0
    i32.store offset=8)
  (func $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$8init_top17h9ba4d179485fee16E (type 5) (param i32 i32 i32)
    (local i32 i32 i32 i32)
    local.get 1
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE
    local.tee 3
    i32.const 8
    call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    local.get 3
    i32.sub
    local.tee 3
    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    local.set 1
    local.get 0
    local.get 2
    local.get 3
    i32.sub
    local.tee 2
    i32.store offset=404
    local.get 0
    local.get 1
    i32.store offset=412
    local.get 1
    local.get 2
    i32.const 1
    i32.or
    i32.store offset=4
    call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
    local.tee 3
    i32.const 8
    call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    local.set 4
    i32.const 20
    i32.const 8
    call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    local.set 5
    i32.const 16
    i32.const 8
    call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
    local.set 6
    local.get 1
    local.get 2
    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
    local.set 1
    local.get 0
    i32.const 2097152
    i32.store offset=440
    local.get 1
    local.get 6
    local.get 5
    local.get 4
    local.get 3
    i32.sub
    i32.add
    i32.add
    i32.store offset=4)
  (func $_ZN3std7process5abort17hf7c8bef35d3938e7E (type 3)
    unreachable
    unreachable)
  (func $__rdl_alloc (type 4) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $_ZN8dlmalloc17Dlmalloc$LT$A$GT$6malloc17he4572c35964f8c9bE)
  (func $__rdl_dealloc (type 5) (param i32 i32 i32)
    i32.const 1048624
    local.get 0
    call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E)
  (func $__rdl_realloc (type 6) (param i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            i32.const 9
            i32.lt_u
            br_if 0 (;@4;)
            local.get 3
            local.get 2
            call $_ZN8dlmalloc17Dlmalloc$LT$A$GT$6malloc17he4572c35964f8c9bE
            local.tee 2
            br_if 1 (;@3;)
            i32.const 0
            return
          end
          call $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE
          local.set 1
          i32.const 0
          local.set 2
          local.get 1
          local.get 1
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.const 20
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.add
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.add
          i32.sub
          i32.const -65544
          i32.add
          i32.const -9
          i32.and
          i32.const -3
          i32.add
          local.tee 1
          i32.const 0
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.const 2
          i32.shl
          i32.sub
          local.tee 4
          local.get 4
          local.get 1
          i32.gt_u
          select
          local.get 3
          i32.le_u
          br_if 1 (;@2;)
          i32.const 16
          local.get 3
          i32.const 4
          i32.add
          i32.const 16
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          i32.const -5
          i32.add
          local.get 3
          i32.gt_u
          select
          i32.const 8
          call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
          local.set 4
          local.get 0
          call $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E
          local.set 1
          local.get 1
          local.get 1
          call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
          local.tee 5
          call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
          local.set 6
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 1
                          call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
                          br_if 0 (;@11;)
                          local.get 5
                          local.get 4
                          i32.ge_u
                          br_if 1 (;@10;)
                          local.get 6
                          i32.const 0
                          i32.load offset=1049036
                          i32.eq
                          br_if 2 (;@9;)
                          local.get 6
                          i32.const 0
                          i32.load offset=1049032
                          i32.eq
                          br_if 3 (;@8;)
                          local.get 6
                          call $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E
                          br_if 7 (;@4;)
                          local.get 6
                          call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                          local.tee 7
                          local.get 5
                          i32.add
                          local.tee 5
                          local.get 4
                          i32.lt_u
                          br_if 7 (;@4;)
                          local.get 5
                          local.get 4
                          i32.sub
                          local.set 8
                          local.get 7
                          i32.const 256
                          i32.lt_u
                          br_if 4 (;@7;)
                          i32.const 1048624
                          local.get 6
                          call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$18unlink_large_chunk17h2484774ef561a518E
                          br 5 (;@6;)
                        end
                        local.get 1
                        call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
                        local.set 5
                        local.get 4
                        i32.const 256
                        i32.lt_u
                        br_if 6 (;@4;)
                        block  ;; label = @11
                          local.get 5
                          local.get 4
                          i32.const 4
                          i32.add
                          i32.lt_u
                          br_if 0 (;@11;)
                          local.get 5
                          local.get 4
                          i32.sub
                          i32.const 131073
                          i32.lt_u
                          br_if 6 (;@5;)
                        end
                        i32.const 1048624
                        local.get 1
                        local.get 1
                        i32.load
                        local.tee 6
                        i32.sub
                        local.get 5
                        local.get 6
                        i32.add
                        i32.const 16
                        i32.add
                        local.tee 7
                        local.get 4
                        i32.const 31
                        i32.add
                        i32.const 1048624
                        call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9page_size17hf5189f015a43cc18E
                        call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                        local.tee 5
                        i32.const 1
                        call $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5remap17h737373babb5822ceE
                        local.tee 4
                        i32.eqz
                        br_if 6 (;@4;)
                        local.get 4
                        local.get 6
                        i32.add
                        local.tee 1
                        local.get 5
                        local.get 6
                        i32.sub
                        local.tee 3
                        i32.const -16
                        i32.add
                        local.tee 2
                        i32.store offset=4
                        call $_ZN8dlmalloc8dlmalloc5Chunk14fencepost_head17h32cfaa035be31489E
                        local.set 0
                        local.get 1
                        local.get 2
                        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                        local.get 0
                        i32.store offset=4
                        local.get 1
                        local.get 3
                        i32.const -12
                        i32.add
                        call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                        i32.const 0
                        i32.store offset=4
                        i32.const 0
                        i32.const 0
                        i32.load offset=1049040
                        local.get 5
                        local.get 7
                        i32.sub
                        i32.add
                        local.tee 3
                        i32.store offset=1049040
                        i32.const 0
                        i32.const 0
                        i32.load offset=1049068
                        local.tee 2
                        local.get 4
                        local.get 4
                        local.get 2
                        i32.gt_u
                        select
                        i32.store offset=1049068
                        i32.const 0
                        i32.const 0
                        i32.load offset=1049044
                        local.tee 2
                        local.get 3
                        local.get 2
                        local.get 3
                        i32.gt_u
                        select
                        i32.store offset=1049044
                        br 9 (;@1;)
                      end
                      local.get 5
                      local.get 4
                      i32.sub
                      local.tee 5
                      i32.const 16
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      i32.lt_u
                      br_if 4 (;@5;)
                      local.get 1
                      local.get 4
                      call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                      local.set 6
                      local.get 1
                      local.get 4
                      call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                      local.get 6
                      local.get 5
                      call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                      local.get 6
                      local.get 5
                      call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
                      br 4 (;@5;)
                    end
                    i32.const 0
                    i32.load offset=1049028
                    local.get 5
                    i32.add
                    local.tee 5
                    local.get 4
                    i32.le_u
                    br_if 4 (;@4;)
                    local.get 1
                    local.get 4
                    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                    local.set 6
                    local.get 1
                    local.get 4
                    call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                    local.get 6
                    local.get 5
                    local.get 4
                    i32.sub
                    local.tee 4
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    i32.const 0
                    local.get 4
                    i32.store offset=1049028
                    i32.const 0
                    local.get 6
                    i32.store offset=1049036
                    br 3 (;@5;)
                  end
                  i32.const 0
                  i32.load offset=1049024
                  local.get 5
                  i32.add
                  local.tee 5
                  local.get 4
                  i32.lt_u
                  br_if 3 (;@4;)
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 5
                      local.get 4
                      i32.sub
                      local.tee 6
                      i32.const 16
                      i32.const 8
                      call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                      i32.ge_u
                      br_if 0 (;@9;)
                      local.get 1
                      local.get 5
                      call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                      i32.const 0
                      local.set 6
                      i32.const 0
                      local.set 5
                      br 1 (;@8;)
                    end
                    local.get 1
                    local.get 4
                    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                    local.tee 5
                    local.get 6
                    call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                    local.set 7
                    local.get 1
                    local.get 4
                    call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                    local.get 5
                    local.get 6
                    call $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E
                    local.get 7
                    call $_ZN8dlmalloc8dlmalloc5Chunk12clear_pinuse17h2a67940d6f74a782E
                  end
                  i32.const 0
                  local.get 5
                  i32.store offset=1049032
                  i32.const 0
                  local.get 6
                  i32.store offset=1049024
                  br 2 (;@5;)
                end
                block  ;; label = @7
                  local.get 6
                  i32.const 12
                  i32.add
                  i32.load
                  local.tee 9
                  local.get 6
                  i32.const 8
                  i32.add
                  i32.load
                  local.tee 6
                  i32.eq
                  br_if 0 (;@7;)
                  local.get 6
                  local.get 9
                  i32.store offset=12
                  local.get 9
                  local.get 6
                  i32.store offset=8
                  br 1 (;@6;)
                end
                i32.const 0
                i32.const 0
                i32.load offset=1048624
                i32.const -2
                local.get 7
                i32.const 3
                i32.shr_u
                i32.rotl
                i32.and
                i32.store offset=1048624
              end
              block  ;; label = @6
                local.get 8
                i32.const 16
                i32.const 8
                call $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E
                i32.lt_u
                br_if 0 (;@6;)
                local.get 1
                local.get 4
                call $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E
                local.set 5
                local.get 1
                local.get 4
                call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                local.get 5
                local.get 8
                call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
                local.get 5
                local.get 8
                call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$13dispose_chunk17h04ab92064f11ad31E
                br 1 (;@5;)
              end
              local.get 1
              local.get 5
              call $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E
            end
            local.get 1
            br_if 3 (;@1;)
          end
          local.get 3
          call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$6malloc17h1bd11c33484481a4E
          local.tee 4
          i32.eqz
          br_if 1 (;@2;)
          local.get 4
          local.get 0
          local.get 3
          local.get 1
          call $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E
          i32.const -8
          i32.const -4
          local.get 1
          call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
          select
          i32.add
          local.tee 2
          local.get 2
          local.get 3
          i32.gt_u
          select
          call $memcpy
          local.set 3
          i32.const 1048624
          local.get 0
          call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E
          local.get 3
          return
        end
        local.get 2
        local.get 0
        local.get 3
        local.get 1
        local.get 1
        local.get 3
        i32.gt_u
        select
        call $memcpy
        drop
        i32.const 1048624
        local.get 0
        call $_ZN8dlmalloc8dlmalloc17Dlmalloc$LT$A$GT$4free17hd094ddfe28573441E
      end
      local.get 2
      return
    end
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE
    drop
    local.get 1
    call $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE)
  (func $_ZN8dlmalloc8dlmalloc8align_up17hd9eacdb194c331e3E (type 4) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add
    i32.const -1
    i32.add
    i32.const 0
    local.get 1
    i32.sub
    i32.and)
  (func $_ZN8dlmalloc8dlmalloc9left_bits17hd43e75bebd2d32bdE (type 2) (param i32) (result i32)
    local.get 0
    i32.const 1
    i32.shl
    local.tee 0
    i32.const 0
    local.get 0
    i32.sub
    i32.or)
  (func $_ZN8dlmalloc8dlmalloc9least_bit17hc868b6f46985b42bE (type 2) (param i32) (result i32)
    i32.const 0
    local.get 0
    i32.sub
    local.get 0
    i32.and)
  (func $_ZN8dlmalloc8dlmalloc24leftshift_for_tree_index17hd789c537cab28411E (type 2) (param i32) (result i32)
    i32.const 0
    i32.const 25
    local.get 0
    i32.const 1
    i32.shr_u
    i32.sub
    local.get 0
    i32.const 31
    i32.eq
    select)
  (func $_ZN8dlmalloc8dlmalloc5Chunk14fencepost_head17h32cfaa035be31489E (type 8) (result i32)
    i32.const 7)
  (func $_ZN8dlmalloc8dlmalloc5Chunk4size17h2c45c180b65c3224E (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=4
    i32.const -8
    i32.and)
  (func $_ZN8dlmalloc8dlmalloc5Chunk6cinuse17h59613f998488ffb3E (type 2) (param i32) (result i32)
    local.get 0
    i32.load8_u offset=4
    i32.const 2
    i32.and
    i32.const 1
    i32.shr_u)
  (func $_ZN8dlmalloc8dlmalloc5Chunk6pinuse17h89f5f80c1a4cb95aE (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=4
    i32.const 1
    i32.and)
  (func $_ZN8dlmalloc8dlmalloc5Chunk12clear_pinuse17h2a67940d6f74a782E (type 0) (param i32)
    local.get 0
    local.get 0
    i32.load offset=4
    i32.const -2
    i32.and
    i32.store offset=4)
  (func $_ZN8dlmalloc8dlmalloc5Chunk5inuse17h4d9d8a6e39f8aee5E (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=4
    i32.const 3
    i32.and
    i32.const 1
    i32.ne)
  (func $_ZN8dlmalloc8dlmalloc5Chunk7mmapped17h56605450e209003dE (type 2) (param i32) (result i32)
    local.get 0
    i32.load8_u offset=4
    i32.const 3
    i32.and
    i32.eqz)
  (func $_ZN8dlmalloc8dlmalloc5Chunk9set_inuse17hc493611b2d4f74f1E (type 1) (param i32 i32)
    local.get 0
    local.get 0
    i32.load offset=4
    i32.const 1
    i32.and
    local.get 1
    i32.or
    i32.const 2
    i32.or
    i32.store offset=4
    local.get 0
    local.get 1
    i32.add
    local.tee 0
    local.get 0
    i32.load offset=4
    i32.const 1
    i32.or
    i32.store offset=4)
  (func $_ZN8dlmalloc8dlmalloc5Chunk20set_inuse_and_pinuse17ha76eb13dcd83db20E (type 1) (param i32 i32)
    local.get 0
    local.get 1
    i32.const 3
    i32.or
    i32.store offset=4
    local.get 0
    local.get 1
    i32.add
    local.tee 0
    local.get 0
    i32.load offset=4
    i32.const 1
    i32.or
    i32.store offset=4)
  (func $_ZN8dlmalloc8dlmalloc5Chunk34set_size_and_pinuse_of_inuse_chunk17h0f4b537b8fccf023E (type 1) (param i32 i32)
    local.get 0
    local.get 1
    i32.const 3
    i32.or
    i32.store offset=4)
  (func $_ZN8dlmalloc8dlmalloc5Chunk33set_size_and_pinuse_of_free_chunk17h74d4897d77859f14E (type 1) (param i32 i32)
    local.get 0
    local.get 1
    i32.const 1
    i32.or
    i32.store offset=4
    local.get 0
    local.get 1
    i32.add
    local.get 1
    i32.store)
  (func $_ZN8dlmalloc8dlmalloc5Chunk20set_free_with_pinuse17h9991ae3d78ae1397E (type 5) (param i32 i32 i32)
    local.get 2
    local.get 2
    i32.load offset=4
    i32.const -2
    i32.and
    i32.store offset=4
    local.get 0
    local.get 1
    i32.const 1
    i32.or
    i32.store offset=4
    local.get 0
    local.get 1
    i32.add
    local.get 1
    i32.store)
  (func $_ZN8dlmalloc8dlmalloc5Chunk11plus_offset17h2f524ce61dfc67e4E (type 4) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add)
  (func $_ZN8dlmalloc8dlmalloc5Chunk12minus_offset17h39dd10694c91288eE (type 4) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.sub)
  (func $_ZN8dlmalloc8dlmalloc5Chunk6to_mem17h6d13f3f3c0bfa5daE (type 2) (param i32) (result i32)
    local.get 0
    i32.const 8
    i32.add)
  (func $_ZN8dlmalloc8dlmalloc5Chunk10mem_offset17h3a01fed98ec6278aE (type 8) (result i32)
    i32.const 8)
  (func $_ZN8dlmalloc8dlmalloc5Chunk8from_mem17h3404f9b5c5e6d4a5E (type 2) (param i32) (result i32)
    local.get 0
    i32.const -8
    i32.add)
  (func $_ZN8dlmalloc8dlmalloc9TreeChunk14leftmost_child17h98469de652a23deaE (type 2) (param i32) (result i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load offset=16
      local.tee 1
      br_if 0 (;@1;)
      local.get 0
      i32.const 20
      i32.add
      i32.load
      local.set 1
    end
    local.get 1)
  (func $_ZN8dlmalloc8dlmalloc9TreeChunk5chunk17h593cb0bc6379ebafE (type 2) (param i32) (result i32)
    local.get 0)
  (func $_ZN8dlmalloc8dlmalloc9TreeChunk4next17h656f2e3867c8acf8E (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=12)
  (func $_ZN8dlmalloc8dlmalloc9TreeChunk4prev17h527f673fd8318adbE (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=8)
  (func $_ZN8dlmalloc8dlmalloc7Segment9is_extern17h775061e2c0d47378E (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=12
    i32.const 1
    i32.and)
  (func $_ZN8dlmalloc8dlmalloc7Segment9sys_flags17h6d168430a1d92f9aE (type 2) (param i32) (result i32)
    local.get 0
    i32.load offset=12
    i32.const 1
    i32.shr_u)
  (func $_ZN8dlmalloc8dlmalloc7Segment5holds17h276a4b63e2947208E (type 4) (param i32 i32) (result i32)
    (local i32 i32)
    i32.const 0
    local.set 2
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 3
      local.get 1
      i32.gt_u
      br_if 0 (;@1;)
      local.get 3
      local.get 0
      i32.load offset=4
      i32.add
      local.get 1
      i32.gt_u
      local.set 2
    end
    local.get 2)
  (func $_ZN8dlmalloc8dlmalloc7Segment3top17he89977119f2095b0E (type 2) (param i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    i32.add)
  (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5alloc17he1272ca423b0b1b4E (type 5) (param i32 i32 i32)
    (local i32)
    local.get 2
    i32.const 16
    i32.shr_u
    memory.grow
    local.set 3
    local.get 0
    i32.const 0
    i32.store offset=8
    local.get 0
    i32.const 0
    local.get 2
    i32.const -65536
    i32.and
    local.get 3
    i32.const -1
    i32.eq
    local.tee 2
    select
    i32.store offset=4
    local.get 0
    i32.const 0
    local.get 3
    i32.const 16
    i32.shl
    local.get 2
    select
    i32.store)
  (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$5remap17h737373babb5822ceE (type 9) (param i32 i32 i32 i32 i32) (result i32)
    i32.const 0)
  (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9free_part17hfe7db7a0188c71a3E (type 6) (param i32 i32 i32 i32) (result i32)
    i32.const 0)
  (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$4free17hc004ad78b71528d6E (type 7) (param i32 i32 i32) (result i32)
    i32.const 0)
  (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$16can_release_part17ha9587956c545036fE (type 4) (param i32 i32) (result i32)
    i32.const 0)
  (func $_ZN61_$LT$dlmalloc..sys..System$u20$as$u20$dlmalloc..Allocator$GT$9page_size17hf5189f015a43cc18E (type 2) (param i32) (result i32)
    i32.const 65536)
  (func $memcpy (type 7) (param i32 i32 i32) (result i32)
    local.get 0
    local.get 1
    local.get 2
    call $_ZN17compiler_builtins3mem6memcpy17h7097b81567cf1b82E)
  (func $_ZN17compiler_builtins3mem6memcpy17h7097b81567cf1b82E (type 7) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.const 15
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        local.set 3
        br 1 (;@1;)
      end
      local.get 0
      i32.const 0
      local.get 0
      i32.sub
      i32.const 3
      i32.and
      local.tee 4
      i32.add
      local.set 5
      block  ;; label = @2
        local.get 4
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.set 3
        local.get 1
        local.set 6
        loop  ;; label = @3
          local.get 3
          local.get 6
          i32.load8_u
          i32.store8
          local.get 6
          i32.const 1
          i32.add
          local.set 6
          local.get 3
          i32.const 1
          i32.add
          local.tee 3
          local.get 5
          i32.lt_u
          br_if 0 (;@3;)
        end
      end
      local.get 5
      local.get 2
      local.get 4
      i32.sub
      local.tee 7
      i32.const -4
      i32.and
      local.tee 8
      i32.add
      local.set 3
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          local.get 4
          i32.add
          local.tee 9
          i32.const 3
          i32.and
          local.tee 6
          i32.eqz
          br_if 0 (;@3;)
          local.get 8
          i32.const 1
          i32.lt_s
          br_if 1 (;@2;)
          local.get 9
          i32.const -4
          i32.and
          local.tee 10
          i32.const 4
          i32.add
          local.set 1
          i32.const 0
          local.get 6
          i32.const 3
          i32.shl
          local.tee 2
          i32.sub
          i32.const 24
          i32.and
          local.set 4
          local.get 10
          i32.load
          local.set 6
          loop  ;; label = @4
            local.get 5
            local.get 6
            local.get 2
            i32.shr_u
            local.get 1
            i32.load
            local.tee 6
            local.get 4
            i32.shl
            i32.or
            i32.store
            local.get 1
            i32.const 4
            i32.add
            local.set 1
            local.get 5
            i32.const 4
            i32.add
            local.tee 5
            local.get 3
            i32.lt_u
            br_if 0 (;@4;)
            br 2 (;@2;)
          end
        end
        local.get 8
        i32.const 1
        i32.lt_s
        br_if 0 (;@2;)
        local.get 9
        local.set 1
        loop  ;; label = @3
          local.get 5
          local.get 1
          i32.load
          i32.store
          local.get 1
          i32.const 4
          i32.add
          local.set 1
          local.get 5
          i32.const 4
          i32.add
          local.tee 5
          local.get 3
          i32.lt_u
          br_if 0 (;@3;)
        end
      end
      local.get 7
      i32.const 3
      i32.and
      local.set 2
      local.get 9
      local.get 8
      i32.add
      local.set 1
    end
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      local.get 2
      i32.add
      local.set 5
      loop  ;; label = @2
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 1
        i32.const 1
        i32.add
        local.set 1
        local.get 3
        i32.const 1
        i32.add
        local.tee 3
        local.get 5
        i32.lt_u
        br_if 0 (;@2;)
      end
    end
    local.get 0)
  (table (;0;) 1 1 funcref)
  (memory (;0;) 17)
  (global $__stack_pointer (mut i32) (i32.const 1048576))
  (global (;1;) i32 (i32.const 1049076))
  (global (;2;) i32 (i32.const 1049088))
  (export "memory" (memory 0))
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
  (export "__data_end" (global 1))
  (export "__heap_base" (global 2))
  (data $.rodata (i32.const 1048576) "Hello, world!"))
