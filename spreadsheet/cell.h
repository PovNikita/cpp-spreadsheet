#pragma once

#include "common.h"
#include "formula.h"
#include <variant>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <string>

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet,  Position pos, std::string&& text);
    ~Cell();

    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void Set(std::string&& text);

    bool IsValidCache() const;
    void InvalidateCache();

    bool IsThereCycleDependency();
    void EraseParentCellFromAllRefferencedCells();
    bool IsThisCellPartOfFormula();
    bool IsFormulaCell();

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;

    Position current_position_;

    std::unordered_set<Position, PositionHasher> parents_cells_;
    mutable std::optional<CellInterface::Value> cache_;

    void InvalidateCacheImpl();

    std::unordered_set<Position, PositionHasher> child_cells_;

    bool CheckCycleDependencyImpl(  std::unordered_set<Position, PositionHasher>& visited, 
                                    std::unordered_set<Position, PositionHasher>& handling_cells);
    
    void SetFormulaImpl(std::string&& text);

    void SetEmptyCellImpl();
    void SetTextCellImpl(std::string&& text);

    CellInterface::Value CalculateValuesImpl() const;

    bool IsTextFormula(std::string_view text) const;

    void EraseRefToThisCellFromChildCell(Position child_cell_pos);
};