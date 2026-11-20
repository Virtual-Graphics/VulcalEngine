#include "resource/pak_provider.h"

#include <miniz.h>

#include <cstring>
#include <fstream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace vulcal::resource {

struct PakProvider::Impl {
    struct Entry {
        mz_uint index;
        std::size_t size;
    };

    std::ifstream file;
    mz_zip_archive archive{};
    std::unordered_map<std::string, Entry> files;
    mutable std::mutex mutex;

    static std::size_t read_at(void* opaque, mz_uint64 offset, void* output, std::size_t size) {
        auto& self = *static_cast<Impl*>(opaque);
        if (offset > static_cast<mz_uint64>(std::numeric_limits<std::streamoff>::max()) ||
            size > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) 
        {
            return 0;
        }

        self.file.clear();
        self.file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!self.file) {
            return 0;
        }
        self.file.read(static_cast<char*>(output), static_cast<std::streamsize>(size));
        return static_cast<std::size_t>(self.file.gcount());
    }

    static std::size_t write_entry(void* opaque, mz_uint64 offset, const void* input, std::size_t size) {
        auto& output = *static_cast<std::vector<std::byte>*>(opaque);
        if (offset > output.size() || size > output.size() - static_cast<std::size_t>(offset)) {
            return 0;
        }
        if (size) { 
            std::memcpy(output.data() + static_cast<std::size_t>(offset), input, size);
        }
        return size;
    }

    ~Impl() {
        if (archive.m_pState) mz_zip_reader_end(&archive);
    }
};

PakProvider::PakProvider(std::filesystem::path pak_file)
        : impl_(std::make_unique<Impl>()) {
    impl_->file.open(pak_file, std::ios::binary);
    if (!impl_->file) {
        throw std::runtime_error("Could not open pak file: " + pak_file.string());
    }

    std::error_code error;
    const auto size = std::filesystem::file_size(pak_file, error);
    if (error) {
        throw std::runtime_error("Invalid pak file: " + pak_file.string());
    }

    impl_->archive.m_pRead            = &Impl::read_at;
    impl_->archive.m_pIO_opaque       = impl_.get();
    impl_->archive.m_pNeeds_keepalive = nullptr;
    if (!mz_zip_reader_init(&impl_->archive, size, 0)) {
        throw std::runtime_error("Could not read pak archive " + pak_file.string() + ": " +
                                 mz_zip_get_error_string(mz_zip_get_last_error(&impl_->archive)));
    }

    std::unordered_set<std::string> all_paths;
    const auto entry_count = mz_zip_reader_get_num_files(&impl_->archive);
    for (mz_uint index = 0; index < entry_count; ++index) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&impl_->archive, index, &stat)) {
            throw std::runtime_error("Could not read a pak entry in " + pak_file.string());
        }
        if (stat.m_is_encrypted || !stat.m_is_supported || stat.m_uncomp_size > std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("Pak contains an encrypted or unsupported entry: " + pak_file.string());
        }

        const auto filename_size = mz_zip_reader_get_filename(&impl_->archive, index, nullptr, 0);
        if (filename_size < 2) throw std::runtime_error("Pak contains an empty entry name: " + pak_file.string());
        std::string filename(filename_size, '\0');
        if (mz_zip_reader_get_filename(&impl_->archive, index, filename.data(), filename_size) != filename_size) {
            throw std::runtime_error("Could not read a pak entry name: " + pak_file.string());
        }
        filename.resize(filename_size - 1);

        const bool has_trailing_slash = !filename.empty() && filename.back() == '/';
        const bool is_directory       = stat.m_is_directory || has_trailing_slash;
        if (has_trailing_slash) filename.pop_back();
        std::string normalized;
        try {
            normalized = detail::normalize_logical_path(filename);
        } catch (const std::invalid_argument& error) {
            throw std::runtime_error("Pak contains an unsafe entry name: " + filename + " (" + error.what() + ")");
        }
        if (!all_paths.insert(normalized).second) {
            throw std::runtime_error("Pak contains a duplicate entry: " + normalized);
        }
        if (!is_directory) impl_->files.emplace(normalized, Impl::Entry{ index, static_cast<std::size_t>(stat.m_uncomp_size) });
    }
}

PakProvider::~PakProvider() = default;

bool PakProvider::contains(std::string_view path) const {
    try {
        const auto normalized = detail::normalize_logical_path(path);
        return impl_->files.contains(normalized);
    } catch (...) {
        return false;
    }
}

std::vector<std::byte> PakProvider::read_binary(std::string_view path) const {
    const auto normalized = detail::normalize_logical_path(path);
    const auto entry      = impl_->files.find(normalized);
    if (entry == impl_->files.end()) throw std::runtime_error("Pak entry does not exist: " + normalized);

    std::lock_guard lock(impl_->mutex);
    std::vector<std::byte> bytes(entry->second.size);
    if (!mz_zip_reader_extract_to_callback(&impl_->archive, entry->second.index, &Impl::write_entry, &bytes, 0)) {
        throw std::runtime_error("Could not extract pak entry " + normalized + ": " +
                                 mz_zip_get_error_string(mz_zip_get_last_error(&impl_->archive)));
    }
    return bytes;
}

std::string PakProvider::read_text(std::string_view path) const {
    const auto bytes = read_binary(path);
    if (bytes.empty()) return {};
    return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
}

} // namespace vulcal::resource
