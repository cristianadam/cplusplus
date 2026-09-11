// RUN: %cxx -fsyntax-only -verify -ast-dump %s | %filecheck %s --match-full-lines

// A constructor and a destructor are declared without a type specifier, and
// the ";" that closes such a declaration belongs to it: left behind it is
// read again as an empty declaration, and the declaration then stops a
// token before its own end.

struct S {
  S();
  ~S();
};

// clang-format off
//      CHECK:translation-unit
// CHECK-NEXT:  declaration-list
// CHECK-NEXT:    simple-declaration
// CHECK-NEXT:      decl-specifier-list
// CHECK-NEXT:        class-specifier
// CHECK-NEXT:          class-key: struct
// CHECK-NEXT:          unqualified-id: name-id
// CHECK-NEXT:            identifier: S
// CHECK-NEXT:          declaration-list
// CHECK-NEXT:            simple-declaration
// CHECK-NEXT:              init-declarator-list
// CHECK-NEXT:                init-declarator
// CHECK-NEXT:                  declarator: declarator
// CHECK-NEXT:                    core-declarator: id-declarator
// CHECK-NEXT:                      unqualified-id: name-id
// CHECK-NEXT:                        identifier: S
// CHECK-NEXT:                    declarator-chunk-list
// CHECK-NEXT:                      function-declarator-chunk
// CHECK-NEXT:            simple-declaration
// CHECK-NEXT:              init-declarator-list
// CHECK-NEXT:                init-declarator
// CHECK-NEXT:                  declarator: declarator
// CHECK-NEXT:                    core-declarator: id-declarator
// CHECK-NEXT:                      unqualified-id: destructor-id
// CHECK-NEXT:                        id: name-id
// CHECK-NEXT:                          identifier: S
// CHECK-NEXT:                    declarator-chunk-list
// CHECK-NEXT:                      function-declarator-chunk
