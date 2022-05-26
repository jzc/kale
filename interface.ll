%Object = type { i64, i64 }

declare void @_print(%Object*, %Object*)
declare void @_make_number(%Object*, double)
declare void @_make_symbol(%Object*, i8*)
declare void @_cons(%Object*, %Object*, %Object*)
declare void @_car(%Object*, %Object*)
declare void @_cdr(%Object*, %Object*)
declare void @_add(%Object*, %Object*, %Object*)
declare void @_sub(%Object*, %Object*, %Object*)
declare void @_mult(%Object*, %Object*, %Object*)
declare void @__div(%Object*, %Object*, %Object*)
declare i1 @_is_nil(%Object*)
declare i8* @_get_code(%Object*, i32)
declare %Object* @_get_fvs(%Object*)
declare void @_create_closure(%Object*, i8*, %Object*, i32, i32)

define %Object @car(%Object %o1) {
  %p1 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  call void @_car(%Object* %pret, %Object* %p1)
  %ret = load %Object, %Object* %pret
  ret %Object %ret  
}

define %Object @cdr(%Object %o1) {
  %p1 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  call void @_cdr(%Object* %pret, %Object* %p1)
  %ret = load %Object, %Object* %pret
  ret %Object %ret  
}

define %Object @get_fv(%Object* %fvs, i32 %i) {
  %ptr = getelementptr %Object, %Object* %fvs, i32 %i
  %ret = load %Object, %Object* %ptr
  ret %Object %ret
}

define %Object @create_closure(i8* %fn_ptr,
       	                       %Object* %fvs, i32 %n_fvs,
			       i32 %n_params) {
  %pret = alloca %Object, align 16
  call void @_create_closure(%Object* %pret, i8* %fn_ptr,
                             %Object* %fvs, i32 %n_fvs,
			     i32 %n_params)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object* @get_fvs(%Object %o1) {
  %p1 = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  %ret = call %Object* @_get_fvs(%Object* %p1)
  ret %Object* %ret
}

define i8* @get_code(%Object %o1, i32 %n) {
  %p1 = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  %ret = call i8* @_get_code(%Object* %p1, i32 %n)
  ret i8* %ret
}

define i1 @is_nil(%Object %o1) {
  %p1 = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  %ret = call i1 @_is_nil(%Object* %p1)
  ret i1 %ret
}

define %Object @make_number(double %d) {
  %pret = alloca %Object, align 16
  call void @_make_number(%Object* %pret, double %d)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @make_symbol(i8* %s) {
  %pret = alloca %Object, align 16
  call void @_make_symbol(%Object* %pret, i8* %s)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @print(%Object %o1) {
  %p1 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  call void @_print(%Object* %pret, %Object* %p1)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @cons(%Object %o1, %Object %o2) {
  %p1 = alloca %Object, align 16
  %p2 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  store %Object %o2, %Object* %p2
  call void @_cons(%Object* %pret, %Object* %p1, %Object* %p2)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @add(%Object %o1, %Object %o2) {
  %p1 = alloca %Object, align 16
  %p2 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  store %Object %o2, %Object* %p2
  call void @_add(%Object* %pret, %Object* %p1, %Object* %p2)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @sub(%Object %o1, %Object %o2) {
  %p1 = alloca %Object, align 16
  %p2 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  store %Object %o2, %Object* %p2
  call void @_sub(%Object* %pret, %Object* %p1, %Object* %p2)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @mult(%Object %o1, %Object %o2) {
  %p1 = alloca %Object, align 16
  %p2 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  store %Object %o2, %Object* %p2
  call void @_mult(%Object* %pret, %Object* %p1, %Object* %p2)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}

define %Object @_div(%Object %o1, %Object %o2) {
  %p1 = alloca %Object, align 16
  %p2 = alloca %Object, align 16
  %pret = alloca %Object, align 16
  store %Object %o1, %Object* %p1
  store %Object %o2, %Object* %p2
  call void @__div(%Object* %pret, %Object* %p1, %Object* %p2)
  %ret = load %Object, %Object* %pret
  ret %Object %ret
}