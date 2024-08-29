#pragma once

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <map>

struct PositionHasher {
    static const uint64_t N = 37;
    size_t operator() (Position pos) const {
        return static_cast<size_t>(pos.row * N + pos.col);
    }
};

class Cell;

class Sheet : public SheetInterface {
public:
    void SetCell(Position pos, std::string text) override;

    // Методы получения ячейки (для которых был вызван SetCell)
    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    // Метод получения ячейки, даже если она пустая
    const CellInterface* GetConcreteCell(Position pos) const;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    bool HasCell(Position pos) const;
    void PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const;

private:
    // Ячейки
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> cells_;
    // Количество элементов в строке: номер строки - количество ячеек, которые у которых выполнен SetCell
    std::map<int, int> row_to_cell_count_;
    // Количество элементов в столбце: номер столбца - количество ячеек, которые у которых выполнен SetCell
    std::map<int, int> column_to_cell_count_;
};