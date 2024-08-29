#include "sheet.h"

#include "cell.h"
#include "formula.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <variant>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    // Если ячейки не существует
    if (!HasCell(pos)) {
        // Создаем ячейку
        cells_[pos] = std::make_unique<Cell>(this);

        // Обновляем данные для вычисления размера печатной области
        ++row_to_cell_count_[pos.row];
        ++column_to_cell_count_[pos.col];
    }

    // Устанавливаем содержимое ячейки
    cells_[pos]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!HasCell(pos) || cells_.at(pos).get()->IsEmpty()) {
        return nullptr;
    }
    return cells_.at(pos).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!HasCell(pos) || cells_.at(pos).get()->IsEmpty()) {
        return nullptr;
    }
    return cells_[pos].get();
}

const CellInterface* Sheet::GetConcreteCell(Position pos) const { 
    if (!HasCell(pos)) {
        return nullptr;
    }
    return cells_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
    // Проверяем наличие ячейки (для которой был вызван SetCell)
    if (!GetCell(pos)) {
        return;
    }
    
    // Обновляем данные для вычисления размера печатной области
    --row_to_cell_count_[pos.row];
    --column_to_cell_count_[pos.col];
    if (row_to_cell_count_.at(pos.row) == 0) {
        row_to_cell_count_.erase(pos.row);
    }
    if (column_to_cell_count_.at(pos.col) == 0) {
        column_to_cell_count_.erase(pos.col);
    }

    // Превращаем ячейку в пустую ячейку
    cells_[pos]->Clear();
}

Size Sheet::GetPrintableSize() const {
    if (row_to_cell_count_.empty()) {
        return {};
    }

    // За счет сортировки в map-ах: 
    // последний элемент в row_to_cell_count_ - это номер самой нижней строки, в которой хранится ячейка, для которой вызван SetCell
    // последний элемент в column_to_cell_count_ - это номер самой последней колонки, в которой хранится ячейка, для которой вызван SetCell
    return {
        row_to_cell_count_.rbegin()->first + 1,
        column_to_cell_count_.rbegin()->first + 1
    };
}

void Sheet::PrintValues(std::ostream& output) const {
    const std::function<void(const CellInterface&)> print_cell = [&output](const CellInterface& cell){
        std::visit([&output](const auto &elem) { output << elem; }, cell.GetValue());
    };
    PrintCells(output, print_cell);
}

void Sheet::PrintTexts(std::ostream& output) const {
    std::function<void(const CellInterface&)> print_cell = [&output](const CellInterface& cell){
        output << cell.GetText();
    };
    PrintCells(output, print_cell);
}

bool Sheet::HasCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("cell check error: position is invalid"s);
    }
    return cells_.count(pos);
}

void Sheet::PrintCells(std::ostream& output, const std::function<void(const CellInterface&)>& printCell) const {
    auto size = GetPrintableSize();
    if (size == Size{}) {
        return;
    }

    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j != 0) {
                output << '\t';
            }
            Position pos{i, j};
            if (!HasCell(pos)) {
                output << ""s;
            } else {
                printCell(*cells_.at(pos));
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() { 
    return std::make_unique<Sheet>(); 
}