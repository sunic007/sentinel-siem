#pragma once

#include "spl/ast.h"
#include "spl/codegen.h"
#include <functional>

namespace sentinel::search {

// Compile an SPL expression into a predicate function
class FilterCompiler {
public:
    static std::function<bool(const spl::Row&)> compile(const spl::Expression& expr);
};

} // namespace sentinel::search
