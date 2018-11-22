#include <string>
#include <storage_engine.h>
#include <table_chunk.h>
#include <configuration.h>
#include <storage_engine_exceptions.h>

namespace st_e {

void StorageEngine::add_table(const Table& table) {
    tables_.save_table(table);
}

//bool StorageEngine::delete_table(std::string table_name) {
//    auto it = tables_.find(table_name);
//    if (it != tables_.end()) {
//        tables_.erase(it);
//        return true;
//    }
//    return false;
//}


const st_e::Table& StorageEngine::get_table_by_name(const std::string& table_name) {
    return tables_.get_table(table_name);
}

StorageEngine &StorageEngine::get_instance() {
    static StorageEngine instance;
    return instance;
}

void StorageEngine::insert(const std::string& table_name, const TableRow& row) {
    const auto& table = get_table_by_name(table_name);

    std::vector<char> record_buffer;

    record_buffer.resize(sizeof(char));
    // first byte is internal value. Value of 0 means not deleted record
    char internal_flag = 0;
    std::memcpy(record_buffer.data(), &internal_flag, sizeof(char));

    for (const auto& cell : row.get_cells()) {
        //todo: add more types
        cell->push_into_buffer(record_buffer);
    }

    auto last_block = get_last_block(table_name);

    if (last_block.get_free_space() < record_buffer.size()) {

        last_block = append_new_block(table_name, last_block);
    }

    append_record_to_block(record_buffer, last_block, table);
}

DataBlock StorageEngine::get_last_block(const std::string& table_name) {
    auto table = tables_.get_table(table_name);
    auto data_file_path = table.get_data_file_path();

    std::fstream data_file;

    data_file.open(table.get_data_file_path().string(), std::fstream::binary | std::fstream::in | std::fstream::out);

    if (!data_file.is_open()) {
        throw TableNotExistException(table.get_name());
    }

    uint32_t first_block_ptr;
    data_file.read(reinterpret_cast<char*>(&first_block_ptr), sizeof(uint32_t));

    // if it is first insert and we have no block created.
    if (first_block_ptr == 0) {
        DataBlock new_data_block(0, 0, 1);
        data_file.seekp(0, std::ios::end);
        auto block_binary = new_data_block.get_binary_representation();
        data_file.write(block_binary.data(), block_binary.size());

        data_file.seekp(0);
        uint32_t new_first_block_pointer = 1;
        data_file.write(reinterpret_cast<const char*>(&new_first_block_pointer), sizeof(uint32_t));
        return new_data_block;
    }

    // if at least one block exists
    uint32_t prev_block_ptr = 0;
    auto next_block_ptr = first_block_ptr;

    uint32_t data_start, free_offset, cur_block_ptr;

    do {
        cur_block_ptr = next_block_ptr;
        long long current_file_offset = DATA_FILE_HEADER_SIZE + (cur_block_ptr - 1) * DATA_BLOCK_SIZE;
        data_file.seekg(current_file_offset);
        data_file.read(reinterpret_cast<char*>(&prev_block_ptr), sizeof(uint32_t));
        data_file.read(reinterpret_cast<char*>(&next_block_ptr), sizeof(uint32_t));
        data_file.read(reinterpret_cast<char*>(&data_start), sizeof(uint32_t));
        data_file.read(reinterpret_cast<char*>(&free_offset), sizeof(uint32_t));
        data_file.read(reinterpret_cast<char*>(&cur_block_ptr), sizeof(uint32_t));
    } while (next_block_ptr);

    return DataBlock(prev_block_ptr, next_block_ptr, data_start, free_offset, cur_block_ptr);
}

void StorageEngine::append_record_to_block(const std::vector<char>& buffer, const DataBlock& block, const Table& table) {
    std::fstream data_file(table.get_data_file_path().string(), std::fstream::binary | std::fstream::in | std::fstream::out);

    if (!data_file.is_open()) {
        throw  TableNotExistException(table.get_name());
    }

    long long free_offset_value_offset = block.get_file_offset() + 12; // 12 because free_offset is 4th uint32_t
    data_file.seekp(block.get_file_offset() + block.get_free_offset());
    data_file.write(buffer.data(), buffer.size());

    data_file.seekp(free_offset_value_offset);
    auto new_free_offset_value = static_cast<uint32_t>(block.get_free_offset() + buffer.size());
    data_file.write(reinterpret_cast<const char*>(&new_free_offset_value), sizeof(uint32_t));
    data_file.close();
}

DataBlock StorageEngine::append_new_block(const std::string& table_name, const DataBlock& last_block) {
    auto table = tables_.get_table(table_name);
    auto data_file_path = table.get_data_file_path();

    std::fstream data_file;

    data_file.open(table.get_data_file_path().string(), std::fstream::binary | std::fstream::in | std::fstream::out);

    if (!data_file.is_open()) {
        throw TableNotExistException(table.get_name());
    }

    // FILL PREV BLOCK WITH TRASH!!!
    data_file.seekp(last_block.get_file_offset() + last_block.get_free_offset());
    char zero = 0;
    for (auto i = last_block.get_free_space(); i > 0; i--) {
        data_file.write(reinterpret_cast<const char*>(&zero), sizeof(char));
    }

    uint32_t block_number;
    data_file.seekg(8);
    data_file.read(reinterpret_cast<char*>(&block_number), sizeof(uint32_t));
    block_number += 1;
    data_file.seekp(8);
    data_file.write(reinterpret_cast<char*>(&block_number), sizeof(uint32_t));

    DataBlock new_data_block(last_block.get_ptr(), 0, block_number);
    data_file.seekp(0, std::ios::end);
    auto block_binary = new_data_block.get_binary_representation();
    data_file.write(block_binary.data(), block_binary.size());

    //change last, block's next_ptr
    data_file.seekp(last_block.get_file_offset() + 4);
    uint32_t curr_block_offset = new_data_block.get_ptr();
    data_file.write(reinterpret_cast<const char*>(&curr_block_offset), sizeof(uint32_t));

    return new_data_block;
}


//SelectAnswer StorageEngine::select(std::string table_name, std::vector<std::string> columns_names) {
//    return tables_.find(table_name)->second->select(columns_names);  //добавить проверку на существование
//}
//
// TODO: continue
//SelectAnswer StorageEngine::select_all(std::string table_name) {
//    auto table = tables_.get_table(table_name);
//    auto data_file_path = table.get_data_file_path();
//
//    std::ifstream data_file;
//    data_file.open(table.get_data_file_path().string(), std::fstream::binary | std::fstream::in);
//
//    return tables_.find(table_name)->second->select_all();
//}

} // namespace st_e