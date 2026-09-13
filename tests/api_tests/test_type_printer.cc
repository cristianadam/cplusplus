// Copyright (c) 2026 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/memory_layout.h>
#include <cxx/name_lookup.h>
#include <cxx/names.h>
#include <cxx/preprocessor.h>
#include <cxx/symbols.h>
#include <cxx/translation_unit.h>
#include <cxx/types.h>
#include <gtest/gtest.h>

using namespace cxx;

TEST(TypePrinter, BasicTypes) {
  Control control;

  ASSERT_EQ(to_string(control.getNullptrType()), "decltype(nullptr)");
  ASSERT_EQ(to_string(control.getDecltypeAutoType()), "decltype(auto)");
  ASSERT_EQ(to_string(control.getAutoType()), "auto");
  ASSERT_EQ(to_string(control.getVoidType()), "void");
  ASSERT_EQ(to_string(control.getBoolType()), "bool");
  ASSERT_EQ(to_string(control.getCharType()), "char");
  ASSERT_EQ(to_string(control.getSignedCharType()), "signed char");
  ASSERT_EQ(to_string(control.getUnsignedCharType()), "unsigned char");
  ASSERT_EQ(to_string(control.getShortIntType()), "short");
  ASSERT_EQ(to_string(control.getUnsignedShortIntType()), "unsigned short");
  ASSERT_EQ(to_string(control.getIntType()), "int");
  ASSERT_EQ(to_string(control.getUnsignedIntType()), "unsigned int");
  ASSERT_EQ(to_string(control.getLongIntType()), "long");
  ASSERT_EQ(to_string(control.getUnsignedLongIntType()), "unsigned long");
  ASSERT_EQ(to_string(control.getLongLongIntType()), "long long");
  ASSERT_EQ(to_string(control.getUnsignedLongLongIntType()),
            "unsigned long long");
  ASSERT_EQ(to_string(control.getFloatType()), "float");
  ASSERT_EQ(to_string(control.getDoubleType()), "double");
  ASSERT_EQ(to_string(control.getLongDoubleType()), "long double");
}

TEST(TypePrinter, QualTypes) {
  Control control;

  ASSERT_EQ(to_string(control.getQualType(control.getUnsignedCharType(),
                                          CvQualifiers::kConst)),
            "const unsigned char");

  ASSERT_EQ(to_string(control.getQualType(control.getUnsignedCharType(),
                                          CvQualifiers::kVolatile)),
            "volatile unsigned char");

  ASSERT_EQ(to_string(control.getQualType(control.getUnsignedCharType(),
                                          CvQualifiers::kConstVolatile)),
            "const volatile unsigned char");
}

TEST(TypePrinter, PointerTypes) {
  Control control;

  ASSERT_EQ(to_string(control.getPointerType(control.getUnsignedCharType())),
            "unsigned char*");

  // test pointer to pointer to unsigned int
  auto pointerToPointerToUnsignedInt = control.getPointerType(
      control.getPointerType(control.getUnsignedIntType()));

  ASSERT_EQ(to_string(pointerToPointerToUnsignedInt), "unsigned int**");

  // test pointer to const char
  auto pointerToConstChar = control.getPointerType(
      control.getQualType(control.getCharType(), CvQualifiers::kConst));

  ASSERT_EQ(to_string(pointerToConstChar), "const char*");

  // const pointer to char
  auto constPointerToChar = control.getQualType(
      control.getPointerType(control.getCharType()), CvQualifiers::kConst);

  ASSERT_EQ(to_string(constPointerToChar), "char* const");

  // pointer to array of 10 ints
  auto pointerToArrayOf10Ints = control.getPointerType(
      control.getBoundedArrayType(control.getIntType(), 10));

  ASSERT_EQ(to_string(pointerToArrayOf10Ints), "int (*)[10]");

  // pointer to function returning int

  auto pointerToFunctionReturningInt =
      control.getPointerType(control.getFunctionType(control.getIntType(), {}));

  ASSERT_EQ(to_string(pointerToFunctionReturningInt), "int (*)()");
}

TEST(TypePrinter, LValueReferences) {
  Control control;

  // lvalue reference to unsigned long
  auto referenceToUnsignedLong =
      control.getLvalueReferenceType(control.getUnsignedLongIntType());

  ASSERT_EQ(to_string(referenceToUnsignedLong), "unsigned long&");

  // lvalue reference to pointer to char
  auto referenceToPointerToChar = control.getLvalueReferenceType(
      control.getPointerType(control.getCharType()));

  ASSERT_EQ(to_string(referenceToPointerToChar), "char*&");

  // lvalue reference to a const pointer to char
  auto referenceToConstPointerToChar =
      control.getLvalueReferenceType(control.getQualType(
          control.getPointerType(control.getCharType()), CvQualifiers::kConst));

  ASSERT_EQ(to_string(referenceToConstPointerToChar), "char* const&");

  // reference to array of 10 ints
  auto referenceToArrayOf10Ints = control.getLvalueReferenceType(
      control.getBoundedArrayType(control.getIntType(), 10));

  ASSERT_EQ(to_string(referenceToArrayOf10Ints), "int (&)[10]");

  // reference to function returning pointer to const char
  auto referenceToFunctionReturningPointerToConstChar =
      control.getLvalueReferenceType(control.getFunctionType(
          control.getPointerType(
              control.getQualType(control.getCharType(), CvQualifiers::kConst)),
          {}));

  ASSERT_EQ(to_string(referenceToFunctionReturningPointerToConstChar),
            "const char* (&)()");
}

TEST(TypePrinter, RValueReferences) {
  Control control;

  // rvalue reference to unsigned long
  auto referenceToUnsignedLong =
      control.getRvalueReferenceType(control.getUnsignedLongIntType());

  ASSERT_EQ(to_string(referenceToUnsignedLong), "unsigned long&&");

  // rvalue reference to pointer to char
  auto referenceToPointerToChar = control.getRvalueReferenceType(
      control.getPointerType(control.getCharType()));

  ASSERT_EQ(to_string(referenceToPointerToChar), "char*&&");
}

TEST(TypePrinter, Arrays) {
  Control control;

  // array of 10 unsigned longs
  auto arrayOf10UnsignedLongs =
      control.getBoundedArrayType(control.getUnsignedLongIntType(), 10);

  ASSERT_EQ(to_string(arrayOf10UnsignedLongs), "unsigned long [10]");

  // array of 4 arrays of 2 floats
  auto arrayOf4ArraysOf2Floats = control.getBoundedArrayType(
      control.getBoundedArrayType(control.getFloatType(), 2), 4);

  ASSERT_EQ(to_string(arrayOf4ArraysOf2Floats), "float [4][2]");

  // array of 4 arrays of 2 pointers to char
  auto arrayOf4ArraysOf2PointersToChar = control.getBoundedArrayType(
      control.getBoundedArrayType(control.getPointerType(control.getCharType()),
                                  2),
      4);

  ASSERT_EQ(to_string(arrayOf4ArraysOf2PointersToChar), "char* [4][2]");

  // array of 4 arrays of 2 pointers to const char
  auto arrayOf4ArraysOf2PointersToConstChar = control.getBoundedArrayType(
      control.getBoundedArrayType(
          control.getPointerType(
              control.getQualType(control.getCharType(), CvQualifiers::kConst)),
          2),
      4);

  ASSERT_EQ(to_string(arrayOf4ArraysOf2PointersToConstChar),
            "const char* [4][2]");

  // array of 4 pointers to function returning int
  auto arrayOf4PointersToFunctionReturningInt = control.getBoundedArrayType(
      control.getPointerType(control.getFunctionType(control.getIntType(), {})),
      4);

  ASSERT_EQ(to_string(arrayOf4PointersToFunctionReturningInt), "int (*[4])()");
}

TEST(TypePrinter, Functions) {
  Control control;

  // function returning pointer to const char

  auto functionReturningPointerToConstChar =
      control.getFunctionType(control.getPointerType(control.getQualType(
                                  control.getCharType(), CvQualifiers::kConst)),
                              {});

  ASSERT_EQ(to_string(functionReturningPointerToConstChar), "const char* ()");

  // function returning const pointer to int

  auto functionReturningConstPointerToInt = control.getFunctionType(
      control.getQualType(control.getPointerType(control.getIntType()),
                          CvQualifiers::kConst),
      {});

  ASSERT_EQ(to_string(functionReturningConstPointerToInt), "int* const ()");

  // variadic function return unsigned long

  auto variadicFunctionReturningUnsignedLong =
      control.getFunctionType(control.getUnsignedLongIntType(), {}, true);

  ASSERT_EQ(to_string(variadicFunctionReturningUnsignedLong),
            "unsigned long (...)");
}
namespace {

class SilentDiagnostics final : public DiagnosticsClient {
 public:
  void report(const Diagnostic&) override {}
};

// The type of the last thing the source declares at namespace scope.
auto lastDeclaredType(const std::string& source, TranslationUnit& unit)
    -> const Type* {
  unit.preprocessor()->setCanResolveFiles(false);
  unit.setSource(source, "test.cc");
  unit.parse({.checkTypes = true});

  const Type* type = nullptr;
  for (Symbol* member : unit.globalScope()->members()) {
    // A function is reached through the overload set it lives in.
    if (auto* overloadSet = symbol_cast<OverloadSetSymbol>(member)) {
      for (auto* function : overloadSet->declaredFunctions()) type = function->type();
      continue;
    }
    if (member->type()) type = member->type();
  }
  return type;
}

}  // namespace

TEST(TypePrinter, QualifiesANameByDefault) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("namespace N { class C {}; } N::C c;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type), "::N::C");
}

TEST(TypePrinter, OmitsTheEnclosingScopeWhenAsked) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("namespace N { class C {}; } N::C c;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "", {.omitEnclosingScope = true}), "C");
}

// The option has to reach the arguments of a template too, or the answer
// is half qualified -- the same as for the parameters of a function.
TEST(TypePrinter, WritesTemplateArgumentsForWhereTheyAreGoing) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "namespace N { class C {}; template <typename T> class List {}; }\n"
      "N::List<N::C> items;",
      unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type), "::N::List<::N::C>");
  EXPECT_EQ(to_string(type, "", {.omitEnclosingScope = true}), "List<C>");
}

// A class writes the name of whatever it inherits without saying where it
// comes from, so a type written for a place inside it needs nothing in
// front of that name either.
TEST(TypePrinter, WritesAnInheritedNameWithNothingInFrontOfIt) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "class Base { public: class Inner {}; };\n"
      "class Derived : public Base { public: Inner held; };\n"
      "Base::Inner outside;\n",
      unit);
  ASSERT_NE(type, nullptr);

  ClassSymbol* derived = nullptr;
  for (Symbol* member : unit.globalScope()->members()) {
    if (auto* klass = symbol_cast<ClassSymbol>(member)) {
      if (klass->name() && to_string(klass->name()) == "Derived") derived = klass;
    }
  }
  ASSERT_NE(derived, nullptr);

  // The same type, written where the class that inherits the name is and
  // where nothing inherits it.
  EXPECT_EQ(to_string(type, "", {.writtenIn = derived}), "Inner");
  EXPECT_EQ(to_string(type), "::Base::Inner");
}

// And nobody can name such a namespace, so what it declares is named from
// the scope around it by its own name alone -- the same as an inherited
// name, and for the same reason: nothing has to be written in front of it.
TEST(TypePrinter, WritesWhatAnUnnamedNamespaceDeclaresByItsOwnName) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "namespace { namespace N { class C {}; } }\n"
      "N::C at_file_scope;\n",
      unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = unit.globalScope()}), "N::C");
}

// A namespace with no name of its own cannot be written, so a path
// through it leaves it out: what reaches such a class is standing in the
// file that declares it, and writing "::" for the namespace would name
// the scope above instead.
TEST(TypePrinter, LeavesOutANamespaceWithNoName) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "namespace { namespace N { class C {}; } }\nN::C c;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type), "::N::C");
}

// A tool with a rule per type matches the type it has a rule for by the
// name the template was declared under: a setting about QList is about
// every QList. It cannot take the arguments off the answer itself, a
// type being written around a name -- "const QList<int> &" has the name
// nowhere near the arguments.
TEST(TypePrinter, OmitsTheTemplateArgumentsWhenAsked) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "template <typename T> class List {}; const List<int> &r = *new List<int>;",
      unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type), "const ::List<int>&");
  EXPECT_EQ(to_string(type, "", {.omitTemplateArguments = true}), "const ::List&");
}

TEST(TypePrinter, OmitsTheEnclosingScopeInsideAFunctionType) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  // The option has to reach the parameters too, or the answer is half
  // qualified.
  const Type* type =
      lastDeclaredType("namespace N { class C {}; } void f(N::C);", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f", {.omitEnclosingScope = true}), "void f(C)");
}

TEST(TypePrinter, WritesAnExceptionSpecificationByDefault) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f() noexcept;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f"), "void f() noexcept");
}

TEST(TypePrinter, OmitsTheExceptionSpecificationWhenAsked) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f() noexcept;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f", {.omitExceptionSpecification = true}), "void f()");
}

namespace {

// The namespace or class \a in declares under \a name.
auto scopeNamed(TranslationUnit& unit, ScopeSymbol* in, std::string_view name)
    -> ScopeSymbol* {
  Symbol* found = qualifiedLookup(in, unit.control()->getIdentifier(name));
  return found ? found->asScopeSymbol() : nullptr;
}

}  // namespace

TEST(TypePrinter, WritesWhatWouldBeWrittenRatherThanThePath) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  // Not ::N::C, which is what the path is: N is reached from where this is
  // going, so N::C is what somebody standing there would write.
  const Type* type =
      lastDeclaredType("namespace N { class C {}; } N::C c;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = unit.globalScope()}), "N::C");
}

TEST(TypePrinter, WritesTheBareNameWhereTheScopeReachesIt) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type =
      lastDeclaredType("namespace N { class C {}; } N::C c;", unit);
  ASSERT_NE(type, nullptr);
  ScopeSymbol* n = scopeNamed(unit, unit.globalScope(), "N");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = n}), "C");
}

TEST(TypePrinter, StopsAtTheFirstNameTheScopeReaches) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  // Standing in A, B is reached and C is not, so B::C is what has to be
  // written: neither the whole path nor the bare name is right here.
  const Type* type = lastDeclaredType(
      "namespace A { namespace B { class C {}; } } A::B::C c;", unit);
  ASSERT_NE(type, nullptr);
  ScopeSymbol* a = scopeNamed(unit, unit.globalScope(), "A");
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = a}), "B::C");
}

TEST(TypePrinter, WritesPastANameThatMeansSomethingElseThere) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  // There is a C in the global namespace as well, and it is not this one, so
  // writing C there would name the wrong class.
  const Type* type = lastDeclaredType(
      "namespace N { class C {}; } class C {}; N::C c;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = unit.globalScope()}), "N::C");
}

TEST(TypePrinter, WritesTheWholePathWhereNothingShorterReachesIt) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  // S has an N of its own, so from in there the namespace cannot be named at
  // all without saying where to start looking. That is what the leading ::
  // is for, and it is the only way the whole path is still written.
  const Type* type = lastDeclaredType(
      "namespace N { class C {}; } struct S { int N; }; N::C c;", unit);
  ASSERT_NE(type, nullptr);
  ScopeSymbol* s = scopeNamed(unit, unit.globalScope(), "S");
  ASSERT_NE(s, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = s}), "::N::C");
}

TEST(TypePrinter, DoesNotWriteAShortNameAUsingDirectiveMadeReachable) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  // A using directive is in force from where it is written onwards, and a
  // scope does not record that. Nor does it say which file it was written
  // in, and an answer written into a header must not lean on what the
  // source file that includes it happens to say.
  const Type* type = lastDeclaredType(
      "namespace N { struct T {}; } using namespace N; struct C { T t; }; T x;",
      unit);
  ASSERT_NE(type, nullptr);
  ScopeSymbol* c = scopeNamed(unit, unit.globalScope(), "C");
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = c}), "N::T");
}

TEST(TypePrinter, WritesAMemberClassFromInsideTheClass) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "namespace N { class C { public: class Inner {}; }; } N::C::Inner x;",
      unit);
  ASSERT_NE(type, nullptr);
  ScopeSymbol* n = scopeNamed(unit, unit.globalScope(), "N");
  ASSERT_NE(n, nullptr);
  ScopeSymbol* c = scopeNamed(unit, n, "C");
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = c}), "Inner");
  EXPECT_EQ(to_string(type, "", {.writtenIn = n}), "C::Inner");
}

TEST(TypePrinter, WritesEachNameOfAFunctionTypeForTheSameScope) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "namespace N { class C {}; } namespace M { class D {}; } N::C f(M::D);",
      unit);
  ASSERT_NE(type, nullptr);
  ScopeSymbol* n = scopeNamed(unit, unit.globalScope(), "N");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(to_string(type, "f", {.writtenIn = n}), "C f(M::D)");
}

TEST(TypePrinter, LeavesTheParametersUnnamedByDefault) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f(int a, double b);", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f"), "void f(int, double)");
}

TEST(TypePrinter, WritesTheParameterNamesItIsGiven) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f(int a, double b);", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f", {.parameterNames = {"a", "b"}}),
            "void f(int a, double b)");
}

TEST(TypePrinter, LeavesAParameterUnnamedWhereNoNameIsGiven) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f(int a, double, char c);", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f", {.parameterNames = {"a", "", "c"}}),
            "void f(int a, double, char c)");
}

// A name is written around the parameter, not after its type, which is the
// whole reason the printer has to do this rather than the caller.
TEST(TypePrinter, WritesAParameterNameInsideItsDeclarator) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f(void (*cb)(int), int a[4]);", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f", {.parameterNames = {"cb", "a"}}),
            "void f(void (*cb)(int), int* a)");
}

// The names are this function's. A function type written inside one of its
// parameters has parameters of its own and they are not these.
TEST(TypePrinter, DoesNotPassTheNamesIntoANestedFunctionType) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void f(void (*cb)(int, int));", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "f", {.parameterNames = {"cb", "x"}}),
            "void f(void (*cb)(int, int))");
}

namespace {

// Reads \a source, so that what it declares can be looked up.
void read(const std::string& source, TranslationUnit& unit) {
  unit.preprocessor()->setCanResolveFiles(false);
  unit.setSource(source, "test.cc");
  unit.parse({.checkTypes = true});
}

// The function \a in declares under \a name, the first of its overloads.
auto functionNamed(TranslationUnit& unit, ScopeSymbol* in, std::string_view name)
    -> Symbol* {
  Symbol* found = qualifiedLookup(in, unit.control()->getIdentifier(name));
  if (auto overloads = symbol_cast<OverloadSetSymbol>(found)) {
    const auto& functions = overloads->declaredFunctions();
    return functions.empty() ? nullptr : functions.front();
  }
  return found;
}

}  // namespace

// What writing a definition somewhere else has to say it is a definition of.
TEST(TypePrinter, WritesAMemberFunctionsNameForOutsideItsClass) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  read("namespace N { struct C { void f(); }; }", unit);
  ScopeSymbol* ns = scopeNamed(unit, unit.globalScope(), "N");
  ASSERT_NE(ns, nullptr);
  ScopeSymbol* cls = scopeNamed(unit, ns, "C");
  ASSERT_NE(cls, nullptr);
  Symbol* function = functionNamed(unit, cls, "f");
  ASSERT_NE(function, nullptr);

  // Standing in the namespace, the class has to be written and N does not.
  EXPECT_EQ(to_string(function, {.writtenIn = ns}), "C::f");

  // Standing outside it, both do.
  EXPECT_EQ(to_string(function, {.writtenIn = unit.globalScope()}), "N::C::f");

  // Standing in the class, neither.
  EXPECT_EQ(to_string(function, {.writtenIn = cls}), "f");
}

// A function is kept in the set of its overloads, which is no scope anybody
// writes -- so it must not turn up in the name.
TEST(TypePrinter, DoesNotWriteTheOverloadSetIntoTheName) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  read("struct C { void f(int); void f(double); };", unit);
  ScopeSymbol* cls = scopeNamed(unit, unit.globalScope(), "C");
  ASSERT_NE(cls, nullptr);
  Symbol* function = functionNamed(unit, cls, "f");
  ASSERT_NE(function, nullptr);

  EXPECT_EQ(to_string(function, {.writtenIn = unit.globalScope()}), "C::f");
}

TEST(TypePrinter, WritesTheWholePathForANameWithNowhereToGo) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  read("namespace N { struct C { void f(); }; }", unit);
  ScopeSymbol* ns = scopeNamed(unit, unit.globalScope(), "N");
  ASSERT_NE(ns, nullptr);
  ScopeSymbol* cls = scopeNamed(unit, ns, "C");
  ASSERT_NE(cls, nullptr);
  Symbol* function = functionNamed(unit, cls, "f");
  ASSERT_NE(function, nullptr);

  // No place named, so the answer is the path, leading :: included: that is
  // what writtenIn means everywhere else too.
  EXPECT_EQ(to_string(function), "::N::C::f");
}

TEST(TypePrinter, WritesNothingForANameThereIsNone) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  EXPECT_EQ(to_string(static_cast<Symbol*>(nullptr)), "");
}

// A specialization is reached by the name of the template it specializes:
// writing "S<int>" finds S, and S<int> is what that names here.
TEST(TypePrinter, WritesASpecializationByTheNameOfItsTemplate) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type =
      lastDeclaredType("template<typename T> struct S {}; S<int> s;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = unit.globalScope()}), "S<int>");
}

TEST(TypePrinter, WritesAClassInsideASpecializationTheSameWay) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType(
      "template<typename T> struct S { struct iterator {}; };"
      "S<int>::iterator it;",
      unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "", {.writtenIn = unit.globalScope()}),
            "S<int>::iterator");
}

// Where the spaces go around a pointer operator is a matter of style, and a
// tool that rewrites declarations to a chosen one cannot work it out from the
// answer afterwards.
TEST(TypePrinter, WritesAPointerOperatorWhereTheStyleAsksFor) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("char *s;", unit);
  ASSERT_NE(type, nullptr);

  EXPECT_EQ(to_string(type, "s"), "char* s");
  EXPECT_EQ(to_string(type, "s",
                      {.spaceBeforePointerOperators = true,
                       .spaceAfterPointerOperators = false}),
            "char *s");
  EXPECT_EQ(to_string(type, "s",
                      {.spaceBeforePointerOperators = true,
                       .spaceAfterPointerOperators = true}),
            "char * s");
  EXPECT_EQ(to_string(type, "s",
                      {.spaceBeforePointerOperators = false,
                       .spaceAfterPointerOperators = false}),
            "char*s");
}

TEST(TypePrinter, WritesAReferenceTheSameWay) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("char c; char &r = c;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "r",
                      {.spaceBeforePointerOperators = true,
                       .spaceAfterPointerOperators = false}),
            "char &r");
}

// A type with nothing to name it after still has the space Overview writes
// there, which is what a list showing types after a colon relies on.
TEST(TypePrinter, WritesAPointerWithNoNameAfterIt) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("char *s;", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "",
                      {.spaceBeforePointerOperators = true,
                       .spaceAfterPointerOperators = true}),
            "char *");
}

// Inside parentheses the spelling is not a choice: the star belongs to what
// follows it, so neither space is written there.
TEST(TypePrinter, LeavesAPointerToAFunctionAlone) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  const Type* type = lastDeclaredType("void (*p)(int);", unit);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(to_string(type, "p",
                      {.spaceBeforePointerOperators = true,
                       .spaceAfterPointerOperators = true}),
            "void (*p)(int)");
}

// A type parameter is a depth and an index: one type stands for the first
// parameter of every template there is, so it carries no name and is printed
// as what it is. Told which declaration the type was read off, the printer
// writes what it is called instead.
TEST(TypePrinter, WritesTemplateParametersByTheirNames) {
  MemoryLayout layout(64);
  SilentDiagnostics diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);

  read("template <class T>\n"
       "struct S {\n"
       "  template <typename U> U convert(T from, U to);\n"
       "};\n",
       unit);

  ScopeSymbol* s = scopeNamed(unit, unit.globalScope(), "S");
  ASSERT_NE(s, nullptr);
  Symbol* convert = functionNamed(unit, s, "convert");
  ASSERT_NE(convert, nullptr);

  // Both lists are in play: T from the class's, U from the function's own.
  EXPECT_EQ(
      to_string(convert->type(), "convert", {.templateParametersOf = convert}),
      "U convert(T, U)");

  // And without being told, what it is rather than what it is called.
  EXPECT_EQ(to_string(convert->type(), "convert"),
            "type-param<0, 1> convert(type-param<0, 0>, type-param<0, 1>)");
}
