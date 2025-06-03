#include "cell.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <optional>
using namespace std::literals;
class Cell::Impl {
public:
    Impl() = default;
    virtual bool IsEmpty() const = 0;
    virtual void Set(std::string&& str) = 0;
    virtual std::string GetText() const = 0;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::vector<Position> GetReferencedCells()const = 0;
    virtual ~Impl() = default;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    EmptyImpl() = default;
    bool IsEmpty() const override{
        return true;
    }
    void Set(std::string&& str) override{
        user_defined_str_ = std::move(str);
    }
    std::string GetText() const override{
        return user_defined_str_;
    }
    CellInterface::Value GetValue() const override{
        return "";
    }
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
    ~EmptyImpl() = default;
private:
    std::string user_defined_str_ = "";
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl() = default;
    ~TextImpl() = default; 
    bool IsEmpty() const override
    {
        return false;
    }
    void Set(std::string&& str) override
    {
        user_defined_str_ = std::move(str);
    }
    std::string GetText() const override {
        return user_defined_str_;
    }
    CellInterface::Value GetValue() const override
    {
        if(user_defined_str_.at(0) == ESCAPE_SIGN)
        {
            return std::string(user_defined_str_.begin() + 1, user_defined_str_.end());
        }
        return user_defined_str_;
    }
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
private:
    std::string user_defined_str_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl() = default;
    FormulaImpl(SheetInterface& sheet) : sheet_(sheet) {}
    ~FormulaImpl() = default;

    bool IsEmpty() const override {
        return false;
    }
    void Set(std::string&& str) override {
        using namespace std::literals;
        SetImpl(str);
        user_defined_str_ = "="s;
        user_defined_str_ += std::move(formula_->GetExpression());
        
    }
    std::string GetText() const override {
        return user_defined_str_;
    }
    CellInterface::Value GetValue() const override {
        try
        {
            value_ = formula_->Evaluate(sheet_);
        }
        catch(...)
        {
            value_ = FormulaError(FormulaError::Category::Arithmetic);
        }
        if(std::holds_alternative<double>(value_))
        {
            return std::get<double>(value_);
        }
        return std::get<FormulaError>(value_);

    }

    std::vector<Position> GetReferencedCells() const override {
        return ref_cells_;
    }

private:
    mutable FormulaInterface::Value value_;
    std::unique_ptr<FormulaInterface> formula_;
    std::string user_defined_str_;
    std::vector<Position> ref_cells_;
    SheetInterface& sheet_;


    FormulaInterface::Value CalculateValuesImpl(Cell* cell);

    void SetImpl(std::string &str)
    {
        try
        {
            formula_ = ParseFormula(std::string(str.begin() + 1, str.end()));
        }
        catch(const FormulaException& exc)
        {
            throw exc;
        }
        ref_cells_ = formula_->GetReferencedCells();
    }
};


void FillChildCellsSet(std::unordered_set<Position, PositionHasher>& child_cells, SheetInterface& sheet, const std::vector<Position>& ref_cells)
{
    for(Position cell_pos : ref_cells)
    {
        if(sheet.GetCell(cell_pos) == nullptr && cell_pos.IsValid())
        {
            sheet.SetCell(cell_pos, ""s);
        }
        child_cells.insert(cell_pos);
    }
}

void Cell::SetFormulaImpl(std::string&& text) {
    std::unique_ptr<Impl> temp_impl = std::move(impl_);
    impl_ = std::make_unique<FormulaImpl>(sheet_);
    try {
        impl_->Set(std::move(text));
    }
    catch (...)
    {
        impl_ = std::move(temp_impl);
        throw;
    }
    std::unordered_set<Position, PositionHasher> temp_child_cell;
    temp_child_cell = std::move(child_cells_);
    child_cells_.clear();
    FillChildCellsSet(child_cells_, sheet_, impl_->GetReferencedCells());
    if(IsThereCycleDependency())
    {
        child_cells_.clear();
        child_cells_ = std::move(temp_child_cell);
        impl_ = std::move(temp_impl);
        throw CircularDependencyException("There is a circular dependency");
    }
    for(Position pos : GetReferencedCells())
    {
        dynamic_cast<Cell*>(sheet_.GetCell(pos))->parents_cells_.insert(current_position_);
    }
    for(Position pos : temp_child_cell)
    {
        if(child_cells_.find(pos) == child_cells_.end())
        {
            //Если ячейки нет в новой формуле, то удалить в этой ячейке упоминание на текущую
            EraseRefToThisCellFromChildCell(pos);
        }
    }
}

Cell::Cell(SheetInterface& sheet, Position pos, std::string&& text)
    : sheet_(sheet)
    , current_position_(pos)
{
    Set(std::move(text));
}

Cell::~Cell() {
}

void Cell::SetEmptyCellImpl() {
    impl_ = std::make_unique<EmptyImpl>();
    child_cells_.clear();
}

void Cell::SetTextCellImpl(std::string&& text) {
    impl_ = std::make_unique<TextImpl>();
    impl_->Set(std::move(text));
    child_cells_.clear();
    cache_ = impl_->GetValue();
}

void Cell::Set(std::string&& text) {
    InvalidateCache();

    if(IsTextFormula(text))
    {
        SetFormulaImpl(std::move(text));
        return;
    }
    if(impl_ && IsTextFormula(impl_->GetText()))
    {
        //Если формульная ячейка становится текстовой или пустой, то разрушаются зависимость этой ячейки от других
        EraseParentCellFromAllRefferencedCells();
    }
    if(text.size() == 0)
    {
        SetEmptyCellImpl();
        return;
    }
    SetTextCellImpl(std::move(text));
}

void Cell::Clear() {
    Set(std::move(""s));
}

Cell::Value Cell::GetValue() const {
    if(!IsValidCache())
    {
        if(IsTextFormula(impl_->GetText())) {
            CalculateValuesImpl();
        }
        else
        {
            return impl_->GetValue();
        }
    }
    return cache_.value();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsValidCache() const
{
    if(cache_.has_value())
    {
        return true;
    }
    return false;
};

void Cell::InvalidateCache()
{
    if(IsValidCache())
    {
        cache_.reset();
        InvalidateCacheImpl();
    }
}

void Cell::InvalidateCacheImpl()
{
    if(!parents_cells_.empty())
    {
        for(auto cell_pos : parents_cells_)
        {
            auto cell_p = sheet_.GetCell(cell_pos);

                dynamic_cast<Cell*>(cell_p)->InvalidateCache();

        }
    }
}

bool Cell::IsThereCycleDependency(){
    std::unordered_set<Position, PositionHasher> visited, handling_cells;
    return CheckCycleDependencyImpl(visited, handling_cells);
}

bool Cell::CheckCycleDependencyImpl(std::unordered_set<Position, PositionHasher>& visited, 
                                    std::unordered_set<Position, PositionHasher>& handling_cells) {
    if(handling_cells.find(current_position_) != handling_cells.end())
    {
        return true;
    }
    if(visited.find(current_position_) != visited.end())
    {
        return false;
    }
    handling_cells.insert(current_position_);
    visited.insert(current_position_);
    if(!child_cells_.empty())
    {
        for(auto cell_pos : child_cells_)
        {
            auto cell_p = sheet_.GetCell(cell_pos);
            if(cell_p != nullptr)
            {
                bool is_cycle_dep = dynamic_cast<Cell*>(cell_p)->CheckCycleDependencyImpl(visited, handling_cells);
                if(is_cycle_dep)
                {
                    return true;
                }
            }
        }
    }
    handling_cells.erase(current_position_);
    return false;
}

CellInterface::Value Cell::CalculateValuesImpl() const
{
    if(!child_cells_.empty())
    {
        for(Position cell_pos : child_cells_)
        {
            if(!cell_pos.IsValid())
            {
                cache_ = FormulaError(FormulaError::Category::Ref);
                return cache_.value();
            }
        }
    }
    cache_ = impl_->GetValue();
    return cache_.value();
}

bool Cell::IsTextFormula(std::string_view text) const{
    if(text.size() == 0)
    {
        return false;
    }
    if(text.at(0) == FORMULA_SIGN && text.size() > 1)
    {
        return true;
    }
    return false;
}

void Cell::EraseParentCellFromAllRefferencedCells()
{
    for(auto cell_pos : child_cells_)
    {
        EraseRefToThisCellFromChildCell(cell_pos);
    }
}

void Cell::EraseRefToThisCellFromChildCell(Position child_cell_pos)
{
    auto cell_p = sheet_.GetCell(child_cell_pos);
    if(cell_p != nullptr)
    {
        Cell* point_to_cell = dynamic_cast<Cell*>(cell_p);
        point_to_cell->parents_cells_.erase(current_position_);
        if(cell_p->GetText().size() == 0 && point_to_cell->parents_cells_.empty())
        {
            //Если ячейка пустая и от неё никто не зависит, то удаляем её.
            sheet_.ClearCell(child_cell_pos);
        }
    }
}

bool Cell::IsThisCellPartOfFormula() {
    if(!parents_cells_.empty())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Cell::IsFormulaCell() {
    return IsTextFormula(GetText());
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    if(std::holds_alternative<double>(value))
    {
        return output << std::get<double>(value);
    }
    else if(std::holds_alternative<std::string>(value))
    {
        return output << std::get<std::string>(value);
    }
    else
    {
        return output << std::get<FormulaError>(value);
    }
}
