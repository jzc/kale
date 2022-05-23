%Object = type { i64, i64 }

declare void @type_error()
declare void @_print(%Object*, %Object*)
declare void @_make_number(%Object*, double)
declare void @_make_symbol(%Object*, i8*)
declare void @_cons(%Object*, %Object*, %Object*)
declare void @_add(%Object*, %Object*, %Object*)
declare void @_sub(%Object*, %Object*, %Object*)
declare void @_mult(%Object*, %Object*, %Object*)
declare void @__div(%Object*, %Object*, %Object*)

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