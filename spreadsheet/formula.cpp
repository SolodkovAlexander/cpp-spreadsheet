#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) :
        ast_(ParseFormulaAST(expression)) 
    {}

    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> get_cell_value = [&sheet](Position pos) {
            if (!pos.IsValid()) {
                throw FormulaErrorException("ref error"s, FormulaError::Category::Ref);
            }
            auto cell = sheet.GetCell(pos);
            if (!cell) {
                return 0.0;
            }
            
            auto cell_value = cell->GetValue();
            if (std::holds_alternative<FormulaError>(cell_value)) {
                throw FormulaErrorException("get value failed"s, std::get<FormulaError>(cell_value).GetCategory());
            }

            double value = 0.0;
            if (std::holds_alternative<double>(cell_value)) {
                value = std::get<double>(cell_value);
            } else {
                std::string value_str = std::get<std::string>(cell_value);
                if (!value_str.empty()) {
                    size_t not_num_pos = 0;
                    try {
                        value = std::stod(std::get<std::string>(cell_value), &not_num_pos);
                    } catch (...) {
                        throw FormulaErrorException("value is not numeric"s, FormulaError::Category::Value);
                    }
                    if (not_num_pos != value_str.size()) {
                        throw FormulaErrorException("value is not numeric"s, FormulaError::Category::Value);
                    }
                }
            }
            return value;
        };
        try {
            return Value(ast_.Execute(get_cell_value));
        } catch (const FormulaErrorException &e) {
            return Value(FormulaError(e.GetCategory()));
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto cells = ast_.GetCells();
        std::vector cells_v(cells.begin(), cells.end());
        auto last = std::unique(cells_v.begin(), cells_v.end());
        cells_v.erase(last, cells_v.end());
        std::sort(cells_v.begin(), cells_v.end());
        return cells_v;
    };

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Parse formula error");
    }    
}