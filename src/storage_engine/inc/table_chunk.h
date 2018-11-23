#include <utility>

#pragma once

#include <utility>
#include <string>
#include "data_block.h"
#include <memory>

namespace st_e {

class TableCell {
public:
    virtual ~TableCell() = default;
    virtual std::string get_data() = 0;
    virtual void push_into_buffer(std::vector<char>& buffer) const = 0;
};

class IntegerTableCell : public TableCell {
public:
    explicit IntegerTableCell(uint32_t data) : data_(data) {}
    std::string get_data() override { return std::to_string(data_); }
    // todo: maybe do like this?
    void push_into_buffer(std::vector<char>& buffer) const override;

private:
    uint32_t data_;
};

class BoolTableCell : public TableCell {
public:
    explicit BoolTableCell(bool data) : data_(data) {}
    std::string get_data() override;
    // todo: maybe do like this?
    void push_into_buffer(std::vector<char>& buffer) const override;

private:
    bool data_;
};

class VarCharTableCell : public TableCell {
public:
    explicit VarCharTableCell(std::string data) : data_(std::move(data)) {}
    std::string get_data() override { return data_; }
    // todo: maybe do like this?
    void push_into_buffer(std::vector<char>& buffer) const override;

private:
    std::string data_;
};

class TableRow {
public:
    explicit TableRow(std::vector<std::shared_ptr<st_e::TableCell>> cells) : cells_(std::move(cells)) {}

//    std::string get_cell(size_t number){ return cells_[number].get_data(); }
    const std::vector<std::shared_ptr<st_e::TableCell>>& get_cells() const { return cells_; }
    bool is_deleted() const { return is_deleted_; }
private:
    std::vector<std::shared_ptr<st_e::TableCell>> cells_;
    bool is_deleted_ = false;
};

class TableChunk {
public:
    TableChunk(const DataBlock& data_block);

};

} // namespace st_e
