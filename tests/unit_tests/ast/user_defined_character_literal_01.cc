// RUN: %cxx -fsyntax-only -verify -ast-dump %s | %filecheck %s --match-full-lines

// A ud-suffix is part of a character literal, the way it is part of an integer
// or floating point literal. Which literal operator the suffix names is not
// resolved yet, so the expression still has the type of the character literal
// itself.

int operator""_X(char);

auto a = 'c'_X;
auto b = L'c'_Y;

// clang-format off
//      CHECK:translation-unit
// CHECK-NEXT:  declaration-list
// CHECK-NEXT:    simple-declaration
// CHECK-NEXT:      decl-specifier-list
// CHECK-NEXT:        integral-type-specifier
// CHECK-NEXT:          specifier: int
// CHECK-NEXT:      init-declarator-list
// CHECK-NEXT:        init-declarator
// CHECK-NEXT:          declarator: declarator
// CHECK-NEXT:            core-declarator: id-declarator
// CHECK-NEXT:              unqualified-id: literal-operator-id
// CHECK-NEXT:                literal: ""_X
// CHECK-NEXT:            declarator-chunk-list
// CHECK-NEXT:              function-declarator-chunk
// CHECK-NEXT:                parameter-declaration-clause: parameter-declaration-clause
// CHECK-NEXT:                  parameter-declaration-list
// CHECK-NEXT:                    parameter-declaration
// CHECK-NEXT:                      type-specifier-list
// CHECK-NEXT:                        integral-type-specifier
// CHECK-NEXT:                          specifier: char
// CHECK-NEXT:    simple-declaration
// CHECK-NEXT:      decl-specifier-list
// CHECK-NEXT:        auto-type-specifier
// CHECK-NEXT:      init-declarator-list
// CHECK-NEXT:        init-declarator
// CHECK-NEXT:          declarator: declarator
// CHECK-NEXT:            core-declarator: id-declarator
// CHECK-NEXT:              unqualified-id: name-id
// CHECK-NEXT:                identifier: a
// CHECK-NEXT:          initializer: equal-initializer [prvalue char]
// CHECK-NEXT:            expression: char-literal-expression [prvalue char]
// CHECK-NEXT:              literal: 'c'_X
// CHECK-NEXT:    simple-declaration
// CHECK-NEXT:      decl-specifier-list
// CHECK-NEXT:        auto-type-specifier
// CHECK-NEXT:      init-declarator-list
// CHECK-NEXT:        init-declarator
// CHECK-NEXT:          declarator: declarator
// CHECK-NEXT:            core-declarator: id-declarator
// CHECK-NEXT:              unqualified-id: name-id
// CHECK-NEXT:                identifier: b
// CHECK-NEXT:          initializer: equal-initializer [prvalue wchar_t]
// CHECK-NEXT:            expression: char-literal-expression [prvalue wchar_t]
// CHECK-NEXT:              literal: L'c'_Y
// clang-format on
