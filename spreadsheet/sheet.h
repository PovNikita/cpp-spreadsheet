#pragma once

#include "common.h"

#include <functional>
#include <unordered_map>
#include <memory>
#include <map>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    void UpdateSize(Position pos, bool IsCellAdded);

private:
    std::unordered_map<Position, std::unique_ptr<CellInterface>, PositionHasher> sheet_;
    std::map<int, int> rows_number_of_elements;
    std::map<int, int> cols_number_of_elements;
};



