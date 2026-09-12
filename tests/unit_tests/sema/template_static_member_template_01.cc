// RUN: %cxx -fsyntax-only -dump-symbols %s | %filecheck %s

// The definition of a member of a class template, written out of line, stands
// under two template parameter lists: the class's and the member's. Nothing is
// declared into either of them, and what is declared belongs to the class.

template <typename T>
struct S {
  static int count;

  template <typename U>
  static U value;

  template <typename U>
  static auto get() -> U;
};

template <typename T>
int S<T>::count = 0;

template <typename T>
template <typename U>
U S<T>::value = U();

template <typename T>
template <typename U>
auto S<T>::get() -> U {
  return U();
}

// clang-format off
//      CHECK:namespace
// CHECK-NEXT:  template class S<type-param<0, 0>>
// CHECK-NEXT:    parameter typename<0, 0> T
// CHECK-NEXT:    injected class name S
// CHECK-NEXT:    field static int count
// CHECK-NEXT:    template variable static type-param<0, 1> value
// CHECK-NEXT:      parameter typename<0, 1> U
// CHECK-NEXT:    template function static type-param<0, 1> get()
// CHECK-NEXT:      parameter typename<0, 1> U
// CHECK-NEXT:      [redeclarations]
// CHECK-NEXT:        template function static type-param<0, 1> get()
// CHECK-NEXT:          parameter typename<0, 1> U
// CHECK-NEXT:          block
// CHECK-NEXT:            variable static constexpr const char __func__[4]

// Nothing of the definitions is left standing beside the class.
// CHECK-NOT:{{^}}  variable
