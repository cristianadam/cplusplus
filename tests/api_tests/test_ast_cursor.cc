#include <cxx/ast.h>
#include <cxx/ast_cursor.h>
#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/memory_layout.h>
#include <cxx/translation_unit.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace cxx;

namespace {

struct Parsed {
  MemoryLayout memoryLayout{64};
  DiagnosticsClient diagnosticsClient;
  TranslationUnit unit{&diagnosticsClient};

  explicit Parsed(std::string source) {
    unit.control()->setMemoryLayout(&memoryLayout);
    unit.setSource(std::move(source), "<test>");
    unit.parse({.checkTypes = true});
  }
};

// The kind of every node the cursor hands back, in the order it hands them
// back. A list is the cursor's own bookkeeping rather than a node.
auto walk(UnitAST* unit) -> std::vector<std::string> {
  std::vector<std::string> kinds;
  for (ASTCursor cursor(unit, "unit"); cursor; ++cursor) {
    auto node = std::get_if<AST*>(&(*cursor).node);
    if (!node || !*node) continue;
    kinds.emplace_back(to_string((*node)->kind()));
  }
  return kinds;
}

auto contains(const std::vector<std::string>& kinds, std::string_view kind)
    -> bool {
  return std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
}

}  // namespace

// Every node is handed back, including the ones whose earlier siblings are
// empty. A statement holds an attribute list before its expression and an
// expression statement rarely has attributes, so a node used to be expanded
// in the same step that dropped an empty slot -- which handed back that
// node's children and never the node itself. Here the statement, the cast in
// it and the name the cast is applied to all follow an empty slot.
TEST(ASTCursor, visitsNodesAfterAnEmptySlot) {
  Parsed parsed{"void f(int a) {\n  (void)a;\n}\n"};

  const auto kinds = walk(parsed.unit.ast());

  ASSERT_TRUE(contains(kinds, "expression-statement"));
  ASSERT_TRUE(contains(kinds, "cast-expression"));
  ASSERT_TRUE(contains(kinds, "id-expression"));
}

// And nothing empty is handed back, so that whatever the cursor is on can be
// read.
TEST(ASTCursor, handsBackNothingEmpty) {
  Parsed parsed{"int f(int a) { return a + 1; }\n"};

  for (ASTCursor cursor(parsed.unit.ast(), "unit"); cursor; ++cursor) {
    const auto& node = (*cursor).node;
    if (auto ast = std::get_if<AST*>(&node)) {
      ASSERT_NE(*ast, nullptr);
    } else if (auto list = std::get_if<List<AST*>*>(&node)) {
      ASSERT_NE(*list, nullptr);
    }
  }
}
