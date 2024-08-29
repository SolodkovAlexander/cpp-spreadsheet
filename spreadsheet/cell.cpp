#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(Sheet* sheet) :
    impl_(std::make_unique<EmptyImpl>()),
    sheet_(sheet)
{}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    // Если происходит установка такого же текста: выход
    if (!IsEmpty() && impl_->GetInitialText() == text) {
        return;
    }

    // Признак, что текст ячейки - формула
    bool is_formula = (!text.empty() && text.at(0) == '=' && text.size() > 1);
    std::unique_ptr<FormulaInterface> formula;
    if (is_formula) {
        // Парсим формулу
        formula = ParseFormula(std::string(text.begin() + 1, text.end()));
        // Выполняем проверку на наличие циклической зависимости в новой формуле
        if (CheckCircularDependency(formula->GetReferencedCells())) {
            throw CircularDependencyException("Invalid formula: found circular dependency"s);
        }
    }

    // Сбрасываем кэш
    InvalidateCache();

    // У ячеек, от которых зависело значение тек. ячейки: убираем связь
    for (auto referenced_cell : GetReferencedCells()) {
        if (!referenced_cell.IsValid()) {
            continue;
        }
        auto cell = dynamic_cast<Cell*>(sheet_->GetCell(referenced_cell));
        if (cell) {
            cell->cells_from_.erase(this);
        }
    }

    // Определяем новую реализацию
    if (is_formula) {
        impl_ = std::make_unique<FormulaImpl>(text, std::move(formula), *sheet_);

        // У ячеек, от которых зависит значение тек. ячейки: устанавливаем связь к тек. ячейке
        for (auto referenced_cell : GetReferencedCells()) {
            if (!referenced_cell.IsValid()) {
                continue;
            }
            // Если ячейки из формулы не существует: создаем пустую ячейку
            auto cell = dynamic_cast<Cell*>(sheet_->GetCell(referenced_cell));
            if (!cell) {
                sheet_->SetCell(referenced_cell, ""s);
                cell = dynamic_cast<Cell*>(sheet_->GetCell(referenced_cell));
            }
            cell->cells_from_.insert(this);
        }
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Clear() {
    // Сбрасываем кэш (рекурсивно)
    InvalidateCache();

    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    auto formula_impl = dynamic_cast<FormulaImpl*>(impl_.get());
    if (!formula_impl) {
        return {};
    }
    return formula_impl->GetReferencedCells();
}

bool Cell::IsEmpty() const {
    return dynamic_cast<EmptyImpl*>(impl_.get());
}

void Cell::InvalidateCache() {
    impl_->InvalidateCache();
    
    // Если были ячейки, которые зависят от текущей ячейки: сбрасываем их кэш значений тоже
    for (auto cell_from : cells_from_) {
        cell_from->InvalidateCache();
    }
}

bool Cell::CheckCircularDependency(const std::vector<Position>& referenced_cells) const {
    for (auto referenced_cell : referenced_cells) {
        std::vector<Position> checked_cells;
        if (CheckCircularDependency(referenced_cell, checked_cells)) {
            return true;
        }
    }
    return false;
}

bool Cell::CheckCircularDependency(Position cell_position, std::vector<Position>& checked_cells) const {
    if (!cell_position.IsValid()) {
        return false;
    }
    auto cell = sheet_->GetConcreteCell(cell_position);
    if (!cell) {
        return false;
    }
    if (cell == this) {
        return true;
    }

    // Помечаем ячейку, что мы её прошли
    checked_cells.push_back(cell_position);

    // Проходим по ячейкам, от которых зависит cell_position
    auto next_cells = cell->GetReferencedCells();
    for (auto next_cell : next_cells) {
        if (std::find(checked_cells.begin(), checked_cells.end(), next_cell) != checked_cells.end()) {
            return true;
        }
        if (CheckCircularDependency(next_cell, checked_cells)) {
            return true;
        }
    }
    return false;
}
