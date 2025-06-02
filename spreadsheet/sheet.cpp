#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if(pos.col >= Position::MAX_COLS || pos.col < 0 || pos.row >= Position::MAX_ROWS || pos.row < 0)
    {
        throw InvalidPositionException("Invalid position");
    }
    if(GetCell(pos) == nullptr)
    {
        sheet_[pos] = std::make_unique<Cell>(*this, pos, std::move(text));
    }
    else if(sheet_[pos]->GetText() == text)
    {
        return;
    }
    else
    {
        dynamic_cast<Cell*>(sheet_[pos].get())->Set(std::move(text));
    }
    UpdateSize(pos, true);

}

void Sheet::UpdateSize(Position pos, bool IsCellAdded) {
    if(IsCellAdded) {
        if(rows_number_of_elements.find(pos.row) == rows_number_of_elements.end())
        {
            rows_number_of_elements[pos.row] = 1;
        }
        else
        {
            rows_number_of_elements[pos.row] += 1;
        }
        if(cols_number_of_elements.find(pos.col) == cols_number_of_elements.end())
        {
            cols_number_of_elements[pos.col] = 1;
        }
        else
        {
            cols_number_of_elements[pos.col] += 1;
        }
    }
    else
    {
        if(rows_number_of_elements[pos.row] == 1)
        {
            rows_number_of_elements.erase(rows_number_of_elements.find(pos.row));
        }
        else
        {
            rows_number_of_elements[pos.row] -= 1;
        }
        if(cols_number_of_elements[pos.col] == 1)
        {
            cols_number_of_elements.erase(cols_number_of_elements.find(pos.col));
        }
        else
        {
            cols_number_of_elements[pos.col] -= 1;
        }
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if(pos.col >= Position::MAX_COLS || pos.col < 0 || pos.row >= Position::MAX_ROWS || pos.row < 0)
    {
        throw InvalidPositionException("Invalid position");
    }
    auto cell_it = sheet_.find(pos);
    if(cell_it == sheet_.end())
    {
        return nullptr;
    }
    return (cell_it->second).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if(pos.col >= Position::MAX_COLS || pos.col < 0 || pos.row >= Position::MAX_ROWS || pos.row < 0)
    {
        throw InvalidPositionException("Invalid position");
    }
    auto cell_it = sheet_.find(pos);
    if(cell_it == sheet_.end())
    {
        return nullptr;
    }
    return (cell_it->second).get();
}

void Sheet::ClearCell(Position pos) {
    if(pos.col >= Position::MAX_COLS || pos.col < 0 || pos.row >= Position::MAX_ROWS || pos.row < 0)
    {
        throw InvalidPositionException("Invalid position");
    }
    if(GetCell(pos) != nullptr)
    {
        sheet_.erase(pos);
        UpdateSize(pos, false);
    }
}

Size Sheet::GetPrintableSize() const {
    int max_row = 0;
    int max_col = 0;
    if(!rows_number_of_elements.empty())
    {
        max_row = (--rows_number_of_elements.end())->first + 1;
    }
    if(!cols_number_of_elements.empty())
    {
        max_col = (--cols_number_of_elements.end())->first + 1;
    }
    return {max_row, max_col};
}

void Sheet::PrintValues(std::ostream& output) const {
    Size print_size = GetPrintableSize();
    for(int y = 0; y < print_size.rows; ++y)
    {
        for(int x = 0; x < print_size.cols - 1; ++x)
        {
            const CellInterface* cell = GetCell({y, x});
            if(cell != nullptr)
            {
                output << cell->GetValue();
                output << "\t"s;
            } 
            else
            {
                output << "\t"s;
            }          
        }
        const CellInterface* cell = GetCell({y, print_size.cols - 1});
        if(cell != nullptr)
        {
            output << cell->GetValue();
        }
        output << "\n"s;
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    Size print_size = GetPrintableSize();
    for(int y = 0; y < print_size.rows; ++y)
    {
        for(int x = 0; x < print_size.cols - 1; ++x)
        {
            const CellInterface* cell = GetCell({y, x});
            if(cell != nullptr)
            {
                output << cell->GetText();
                output << "\t"s;
            }
            else
            {
                output << "\t"s;
            }           
        }
        const CellInterface* cell = GetCell({y, print_size.cols - 1});
        if(cell != nullptr)
        {
            output << cell->GetText();
        }
        output << "\n"s;
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}