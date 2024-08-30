#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <algorithm>
#include <optional>
#include <unordered_set>

using namespace std::literals;

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet* sheet);
    Cell(Cell&&) = default;
    Cell& operator=(Cell&&) = default;
    ~Cell();

public:
    void Set(std::string text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;    
    std::vector<Position> GetReferencedCells() const override;
    bool IsEmpty() const;

private:
    class Impl {
        public:
            Impl() = default;
            virtual ~Impl() = default;

        public:
            virtual Value GetValue() const = 0;
            virtual std::string GetText() const = 0;
            virtual std::string GetInitialText() const = 0;
            virtual void InvalidateCache() const {}
    };
    class EmptyImpl final : public Impl {
        public:
            Value GetValue() const override { return ""s; }
            std::string GetText() const override { return ""s; }
            std::string GetInitialText() const override { return ""s; }
    };
    class TextImpl final : public Impl {
        public:
            TextImpl(std::string text) :
                text_(text) 
            {}

        public:
            Value GetValue() const override { 
                return (text_.size() > 0 && text_.at(0) == '\''
                        ? std::string(text_.begin() + 1, text_.end())
                        : text_);
            }
            std::string GetText() const override { return text_; }
            std::string GetInitialText() const override { return text_; }

        private: 
            std::string text_;
    };
    class FormulaImpl final : public Impl {
        private:
            // Символ начала формулы в тексте
            static const char FORMULA_SIGN = '=';

        public:
            FormulaImpl(std::string text, const SheetInterface& sheet) :
                text_(text),
                formula_(std::move(ParseFormula(std::string(text.begin() + 1, text.end())))),
                sheet_(sheet) 
            {}
            
        public:
            Value GetValue() const override {
                if (value_cache_) {
                    return *value_cache_;
                }
                
                FormulaInterface::Value value = formula_->Evaluate(sheet_);
                if (std::holds_alternative<double>(value)) {
                    value_cache_ = std::get<double>(value);
                } else {
                    value_cache_ = std::get<FormulaError>(value);
                }
                return *value_cache_;
            }
            std::string GetText() const override { return '=' + formula_->GetExpression(); }
            std::string GetInitialText() const override { return text_; }
            void InvalidateCache() const override { value_cache_ = std::nullopt; }
            std::vector<Position> GetReferencedCells() const { return formula_->GetReferencedCells(); }
            static bool IsFormulaText(std::string text) { return (!text.empty() && text.at(0) == FORMULA_SIGN && text.size() > 1); };

        private: 
            std::string text_;
            std::unique_ptr<FormulaInterface> formula_;
            // таблица ячейки 
            // (необходима для получения доступа к ячейкам в случае формульных ячеек, содержащих в формулах индексы на ячейки)
            const SheetInterface& sheet_;
            // Кэш вычисленного значения
            mutable std::optional<Value> value_cache_;
    };

private:
    void InvalidateCache();
    void ClearLinksFrom();
    void CreateLinksFrom();
    bool CheckCircularDependency(const std::vector<Position>& referenced_cells) const;
    bool CheckCircularDependency(Position cell_position, std::vector<Position>& checked_cells) const;

private:
    std::unique_ptr<Impl> impl_;
    // таблица ячейки
    Sheet* sheet_;
    // ячейки, которые ссылаются на текущую ячейку (т.е. ячейки, чье вычисление значения зависит от текущей ячейки)
    // (необходим для инвалидации кэша)
    std::unordered_set<Cell*> cells_from_;
};