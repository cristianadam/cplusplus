// RUN: %cxx -fsyntax-only -verify -ast-dump %s | %filecheck %s --match-full-lines

// The attribute-specifier-seq of a labeled statement precedes the label
// ([stmt.pre]) and appertains to it.

void f(int j) {
  switch (j) {
    [[likely]] case 1:
      break;
    [[unlikely]] default:
      break;
  }

  [[maybe_unused]] done:
  return;
}

// clang-format off
//      CHECK:translation-unit
// CHECK-NEXT:  declaration-list
// CHECK-NEXT:    function-definition
// CHECK-NEXT:      decl-specifier-list
// CHECK-NEXT:        void-type-specifier
// CHECK-NEXT:      declarator: declarator
// CHECK-NEXT:        core-declarator: id-declarator
// CHECK-NEXT:          unqualified-id: name-id
// CHECK-NEXT:            identifier: f
// CHECK-NEXT:        declarator-chunk-list
// CHECK-NEXT:          function-declarator-chunk
// CHECK-NEXT:            parameter-declaration-clause: parameter-declaration-clause
// CHECK-NEXT:              parameter-declaration-list
// CHECK-NEXT:                parameter-declaration
// CHECK-NEXT:                  identifier: j
// CHECK-NEXT:                  type-specifier-list
// CHECK-NEXT:                    integral-type-specifier
// CHECK-NEXT:                      specifier: int
// CHECK-NEXT:                  declarator: declarator
// CHECK-NEXT:                    core-declarator: id-declarator
// CHECK-NEXT:                      unqualified-id: name-id
// CHECK-NEXT:                        identifier: j
// CHECK-NEXT:      function-body: compound-statement-function-body
// CHECK-NEXT:        statement: compound-statement
// CHECK-NEXT:          statement-list
// CHECK-NEXT:            switch-statement
// CHECK-NEXT:              condition: implicit-cast-expression [prvalue int]
// CHECK-NEXT:                cast-kind: lvalue-to-rvalue-conversion
// CHECK-NEXT:                expression: id-expression [lvalue int]
// CHECK-NEXT:                  unqualified-id: name-id
// CHECK-NEXT:                    identifier: j
// CHECK-NEXT:              statement: compound-statement
// CHECK-NEXT:                statement-list
// CHECK-NEXT:                  case-statement
// CHECK-NEXT:                    attribute-list
// CHECK-NEXT:                      cxx-attribute
// CHECK-NEXT:                        attribute-list
// CHECK-NEXT:                          attribute
// CHECK-NEXT:                            attribute-token: simple-attribute-token
// CHECK-NEXT:                              identifier: likely
// CHECK-NEXT:                    expression: int-literal-expression [prvalue int]
// CHECK-NEXT:                      literal: 1
// CHECK-NEXT:                  break-statement
// CHECK-NEXT:                  default-statement
// CHECK-NEXT:                    attribute-list
// CHECK-NEXT:                      cxx-attribute
// CHECK-NEXT:                        attribute-list
// CHECK-NEXT:                          attribute
// CHECK-NEXT:                            attribute-token: simple-attribute-token
// CHECK-NEXT:                              identifier: unlikely
// CHECK-NEXT:                  break-statement
// CHECK-NEXT:            labeled-statement
// CHECK-NEXT:              identifier: done
// CHECK-NEXT:              attribute-list
// CHECK-NEXT:                cxx-attribute
// CHECK-NEXT:                  attribute-list
// CHECK-NEXT:                    attribute
// CHECK-NEXT:                      attribute-token: simple-attribute-token
// CHECK-NEXT:                        identifier: maybe_unused
// CHECK-NEXT:              statement: return-statement
// clang-format on
