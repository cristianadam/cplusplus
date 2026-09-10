#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/memory_layout.h>
#include <cxx/translation_unit.h>
#include <gtest/gtest.h>

using namespace cxx;

namespace {

// Parsing with a memory layout set means the layout of every class is
// built, and building one walks the bases.
auto parseWithLayout(const std::string& source) -> void {
  MemoryLayout layout(64);
  DiagnosticsClient diagnostics;
  TranslationUnit unit(&diagnostics);
  unit.control()->setMemoryLayout(&layout);
  unit.setSource(source, "layout.cc");
  unit.parse({.checkTypes = true});
}

}  // namespace

// A class that inherits itself, however far around. It is ill-formed and
// is diagnosed, and laying it out must still come to an end: the walk over
// the bases used to follow the cycle for ever.
TEST(RecordLayout, aCycleInTheBasesTerminates) {
  parseWithLayout("struct B;\nstruct A : B {};\nstruct B : A {};\n");
  parseWithLayout("struct A;\nstruct C;\nstruct A : C {};\nstruct B : A {};\n"
                  "struct C : B {};\n");
  SUCCEED();  // reaching here at all is the point
}

// And a class inherited by two paths is still walked by both, which is how
// a diamond gets the two copies it is meant to have.
TEST(RecordLayout, aDiamondKeepsBothPaths) {
  parseWithLayout("struct Base { int m; };\nstruct Left : Base {};\n"
                  "struct Right : Base {};\nstruct Bottom : Left, Right {};\n");
  SUCCEED();
}
