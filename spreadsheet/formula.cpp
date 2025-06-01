#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    if(fe.GetCategory() == FormulaError::Category::Arithmetic)
    {
        return output << "#ARITHM!";
    }
    else if(fe.GetCategory() == FormulaError::Category::Value)
    {
        return output << "#VALUE!";
    }
    return output << "#REF!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) 
    try
        : ast_(ParseFormulaAST(expression)) {
    } catch (...) {
        throw FormulaException ("The formula is incorrect");
    }
    Value Evaluate(const SheetInterface& sheet) const override {
        Value val;
        try
        {
            val = ast_.Execute(sheet);
        }
        catch(const FormulaError& e)
        {
            val = e;
        }
        return val;
    } 
    std::string GetExpression() const override {
        std::stringstream str_stream;
        ast_.PrintFormula(str_stream);
        return str_stream.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> ref_cells = ast_.GetReferencedCells();
        std::sort(ref_cells.begin(), ref_cells.end(), [](const Position& lhs, const Position& rhs) {
                                                            return lhs < rhs;
        });
        auto last = std::unique(ref_cells.begin(), ref_cells.end());
        ref_cells.erase(last, ref_cells.end());
        return ref_cells;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}