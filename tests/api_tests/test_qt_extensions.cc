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
#include <cxx/name_lookup.h>
#include <cxx/names.h>
#include <cxx/preprocessor.h>
#include <cxx/symbols.h>
#include <cxx/translation_unit.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace cxx;

namespace {

// What was said about the file itself. The front end declares a preamble of
// its own before reading a line of it, and what it says about that is not
// this test's business.
class RecordingDiagnosticsClient final : public DiagnosticsClient {
 public:
  void report(const Diagnostic& diagnostic) override {
    auto* pp = preprocessor();
    if (!pp) return;
    const auto position = pp->tokenStartPosition(diagnostic.token());
    if (position.fileName != "test.cc") return;
    messages.push_back(diagnostic.message());
  }

  std::vector<std::string> messages;
};

// A file written in Qt, read with or without knowing what Qt writes.
struct QtSource {
  MemoryLayout memoryLayout{64};
  RecordingDiagnosticsClient diagnosticsClient;
  TranslationUnit unit{&diagnosticsClient};

  QtSource(const std::string& source, bool qtExtensions) {
    unit.control()->setMemoryLayout(&memoryLayout);
    unit.preprocessor()->setCanResolveFiles(false);
    unit.preprocessor()->setQtExtensions(qtExtensions);
    unit.setSource(source, "test.cc");
    unit.parse({.checkTypes = true});
  }

  auto classNamed(std::string_view name) -> ClassSymbol* {
    auto id = unit.control()->getIdentifier(name);
    for (auto candidate : unit.globalScope()->find(id)) {
      if (auto classSymbol = symbol_cast<ClassSymbol>(candidate))
        return classSymbol;
    }
    return nullptr;
  }

  auto methodNamed(ClassSymbol* classSymbol, std::string_view name)
      -> FunctionSymbol* {
    auto id = unit.control()->getIdentifier(name);
    for (auto candidate : classSymbol->find(id)) {
      if (auto overloads = symbol_cast<OverloadSetSymbol>(candidate)) {
        const auto& functions = overloads->declaredFunctions();
        if (!functions.empty()) return functions.front();
      }
      if (auto function = symbol_cast<FunctionSymbol>(candidate)) return function;
    }
    return nullptr;
  }
};

const std::string kWidget = R"(
class QObject {};

class Widget : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    int value() const;

signals:
    void valueChanged(int v);

public slots:
    void reset();

private slots:
    void tick();

public:
    Q_INVOKABLE void poke();
    void plain();
};

void use(Widget *w) { emit w->valueChanged(1); }
)";

// The same class, read where Qt's own headers have been: every one of the
// words is a macro there, defined to expand to nothing or to an access
// specifier, which is how they reach a compiler that knows nothing of Qt.
const std::string kWidgetWithQtMacros = R"(
#define QT_ANNOTATE_ACCESS_SPECIFIER(x)
#define QT_ANNOTATE_CLASS(type, ...)
#define slots Q_SLOTS
#define signals Q_SIGNALS
#define Q_SLOTS QT_ANNOTATE_ACCESS_SPECIFIER(qt_slot)
#define Q_SIGNALS public QT_ANNOTATE_ACCESS_SPECIFIER(qt_signal)
#define Q_PROPERTY(...) QT_ANNOTATE_CLASS(qt_property, __VA_ARGS__)
#define Q_INVOKABLE QT_ANNOTATE_FUNCTION(qt_invokable)
#define QT_ANNOTATE_FUNCTION(x)
#define Q_OBJECT
#define emit
)" + kWidget;

}  // namespace

// What a file written in Qt is, read as C++: a class body with words in it
// where a declaration should be.
TEST(QtExtensions, AreNotReadUnlessAskedFor) {
  QtSource source{kWidget, /*qtExtensions=*/false};
  ASSERT_FALSE(source.diagnosticsClient.messages.empty());
}

// And read, with nothing left to complain about.
TEST(QtExtensions, LetAFileWrittenInQtBeRead) {
  QtSource source{kWidget, /*qtExtensions=*/true};

  for (const auto& message : source.diagnosticsClient.messages) {
    ADD_FAILURE() << "unexpected diagnostic: " << message;
  }
}

TEST(QtExtensions, SayWhatQtMakesOfEachMember) {
  QtSource source{kWidget, /*qtExtensions=*/true};

  auto* widget = source.classNamed("Widget");
  ASSERT_NE(widget, nullptr);
  EXPECT_TRUE(widget->isQObject());
  EXPECT_FALSE(widget->isQGadget());

  const auto kindOf = [&](std::string_view name) {
    auto* method = source.methodNamed(widget, name);
    return method ? method->qtMethodKind() : QtMethodKind::kNone;
  };

  EXPECT_EQ(kindOf("valueChanged"), QtMethodKind::kSignal);
  EXPECT_EQ(kindOf("reset"), QtMethodKind::kSlot);
  EXPECT_EQ(kindOf("tick"), QtMethodKind::kSlot);
  EXPECT_EQ(kindOf("poke"), QtMethodKind::kInvokable);

  // The section a slot stands in ends where the next one begins, so what
  // follows is a member like any other.
  EXPECT_EQ(kindOf("plain"), QtMethodKind::kNone);
  EXPECT_EQ(kindOf("value"), QtMethodKind::kNone);
}

// A signal is callable by anybody, whatever stood in front of the section;
// a slot keeps the access the class gave it.
TEST(QtExtensions, SayWhoMayCallWhat) {
  QtSource source{kWidget, /*qtExtensions=*/true};

  auto* widget = source.classNamed("Widget");
  ASSERT_NE(widget, nullptr);

  EXPECT_EQ(source.methodNamed(widget, "valueChanged")->accessSpecifier(),
            AccessSpecifier::kPublic);
  EXPECT_EQ(source.methodNamed(widget, "reset")->accessSpecifier(),
            AccessSpecifier::kPublic);
  EXPECT_EQ(source.methodNamed(widget, "tick")->accessSpecifier(),
            AccessSpecifier::kPrivate);
}

// And what they are worth where the headers have already turned them into
// nothing: a tool that lets them expand cannot tell a signal from any
// other member, which is why they are left where they are written.
TEST(QtExtensions, SurviveTheMacrosQtDefinesThemAs) {
  QtSource expanded{kWidgetWithQtMacros, /*qtExtensions=*/false};
  auto* widgetAsCxx = expanded.classNamed("Widget");
  ASSERT_NE(widgetAsCxx, nullptr);
  EXPECT_EQ(expanded.methodNamed(widgetAsCxx, "valueChanged")->qtMethodKind(),
            QtMethodKind::kNone);

  QtSource source{kWidgetWithQtMacros, /*qtExtensions=*/true};
  for (const auto& message : source.diagnosticsClient.messages) {
    ADD_FAILURE() << "unexpected diagnostic: " << message;
  }

  auto* widget = source.classNamed("Widget");
  ASSERT_NE(widget, nullptr);
  EXPECT_TRUE(widget->isQObject());
  EXPECT_EQ(source.methodNamed(widget, "valueChanged")->qtMethodKind(),
            QtMethodKind::kSignal);
  EXPECT_EQ(source.methodNamed(widget, "reset")->qtMethodKind(),
            QtMethodKind::kSlot);
  EXPECT_EQ(source.methodNamed(widget, "poke")->qtMethodKind(),
            QtMethodKind::kInvokable);
}
