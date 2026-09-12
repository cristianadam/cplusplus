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
#include <cxx/diagnostic.h>
#include <cxx/diagnostics_client.h>
#include <cxx/memory_layout.h>
#include <cxx/translation_unit.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace cxx;

namespace {

// Keeps what was reported instead of printing it, so a test can say what the
// front end made of a source rather than only that it survived it.
class RecordingDiagnosticsClient final : public DiagnosticsClient {
 public:
  void report(const Diagnostic& diagnostic) override {
    messages.push_back(diagnostic.message());
  }

  std::vector<std::string> messages;
};

}  // namespace

// A template that instantiates itself never stops on its own. What stops it
// has to be reached while there is still stack to report it with: every level
// is a deep native call, through the rewriter and the binder and back.
TEST(InstantiationDepth, RecursiveInstantiationIsReported) {
  RecordingDiagnosticsClient client;
  MemoryLayout memoryLayout{64};
  TranslationUnit unit{&client};
  unit.control()->setMemoryLayout(&memoryLayout);

  unit.setSource(
      "template <int N>\n"
      "struct R {\n"
      "  using type = typename R<N + 1>::type;\n"
      "};\n"
      "\n"
      "using T = R<0>::type;\n",
      "<test>");

  unit.parse({.checkTypes = true});

  const bool stopped =
      std::any_of(client.messages.begin(), client.messages.end(),
                  [](const std::string& message) {
                    return message.find(
                               "recursive template instantiation exceeded "
                               "maximum depth") != std::string::npos;
                  });

  ASSERT_TRUE(stopped) << "the recursion was not stopped";
}
