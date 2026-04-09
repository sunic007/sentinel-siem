#include "search/filter.h"

namespace sentinel::search {

std::function<bool(const spl::Row&)> FilterCompiler::compile(const spl::Expression& expr) {
    if (auto* cmp = dynamic_cast<const spl::CompareExpr*>(&expr)) {
        auto* left_field = dynamic_cast<const spl::FieldExpr*>(cmp->left.get());
        auto* right_lit = dynamic_cast<const spl::LiteralExpr*>(cmp->right.get());

        if (left_field && right_lit) {
            std::string field = left_field->field_name;
            std::string value = right_lit->value;
            auto op = cmp->op;

            return [field, value, op](const spl::Row& row) -> bool {
                auto it = row.find(field);
                if (it == row.end()) return false;
                const auto& actual = it->second;

                switch (op) {
                    case spl::CompareOp::EQ:  return actual == value;
                    case spl::CompareOp::NEQ: return actual != value;
                    case spl::CompareOp::LT:  return actual < value;
                    case spl::CompareOp::GT:  return actual > value;
                    case spl::CompareOp::LTE: return actual <= value;
                    case spl::CompareOp::GTE: return actual >= value;
                    default: return false;
                }
            };
        }
    }

    if (auto* logic = dynamic_cast<const spl::LogicExpr*>(&expr)) {
        if (logic->op == spl::LogicOp::NOT && logic->left) {
            auto inner = compile(*logic->left);
            return [inner](const spl::Row& row) { return !inner(row); };
        }

        if (logic->left && logic->right) {
            auto left_fn = compile(*logic->left);
            auto right_fn = compile(*logic->right);

            if (logic->op == spl::LogicOp::AND) {
                return [left_fn, right_fn](const spl::Row& row) {
                    return left_fn(row) && right_fn(row);
                };
            } else {
                return [left_fn, right_fn](const spl::Row& row) {
                    return left_fn(row) || right_fn(row);
                };
            }
        }
    }

    // Default: pass everything
    return [](const spl::Row&) { return true; };
}

} // namespace sentinel::search
