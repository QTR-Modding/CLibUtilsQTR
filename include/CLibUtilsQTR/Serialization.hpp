#pragma once
#include <SKSE/SKSE.h>
#include <array>
#include <functional>
#include <limits>
#include <type_traits>
#include <map>
#include <mutex>

namespace Serialization {
    // Credits: https:// github.com/ozooma10/OSLAroused/blob/29ac62f220fadc63c829f6933e04be429d4f96b0/src/PersistedData.cpp
    // BaseData is based off how powerof3's did it in Afterlife
    template <typename T, typename U>
    class BaseData {
    public:
        float GetData(T formId, T missing) {
            Locker locker(m_Lock);
            // if the plugin version is less than 0.7 need to handle differently
            // if (SKSE::PluginInfo::version)
            if (auto idx = m_Data.contains(formId)) {
                return m_Data[formId];
            }
            return missing;
        }

        void SetData(T formId, U value) {
            Locker locker(m_Lock);
            m_Data[formId] = value;
        }

        virtual const char* GetType() = 0;

        virtual bool Save(SKSE::SerializationInterface*, std::uint32_t,
                          std::uint32_t) { return false; }

        virtual bool Save(SKSE::SerializationInterface*) { return false; }
        virtual bool Load(SKSE::SerializationInterface*) { return false; }

        void Clear() {
            Locker locker(m_Lock);
            m_Data.clear();
        }

    protected:
        ~BaseData() = default;
        std::map<T, U> m_Data;

        using Lock = std::recursive_mutex;
        using Locker = std::lock_guard<Lock>;
        mutable Lock m_Lock;
    };

    inline std::vector<std::pair<int, bool>> encodeString(const std::string& inputString) {
        std::vector<std::pair<int, bool>> encodedValues;
        try {
            for (int i = 0; i < 100 && inputString[i] != '\0'; i++) {
                const char ch = inputString[i];
                if (std::isprint(ch) && (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) && ch >= 0) {
                    encodedValues.emplace_back(static_cast<int>(ch), std::isupper(ch));
                }
            }
        } catch (const std::exception& e) {
            SKSE::log::error("Error encoding string: {}", e.what());
            return encodeString("ERROR");
        }
        return encodedValues;
    }

    inline std::string decodeString(const std::vector<std::pair<int, bool>>& encodedValues) {
        std::string decodedString;
        for (const auto& pair : encodedValues) {
            const char ch = static_cast<char>(pair.first);
            if (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) {
                if (pair.second) {
                    decodedString += ch;
                } else {
                    decodedString += static_cast<char>(std::tolower(ch));
                }
            }
        }
        return decodedString;
    }

    inline bool read_string(SKSE::SerializationInterface* a_intfc, std::string& a_str) {
        std::vector<std::pair<int, bool>> encodedStr;
        std::size_t size;
        if (a_intfc->ReadRecordData(size) != sizeof(size)) {
            return false;
        }
        for (std::size_t i = 0; i < size; i++) {
            std::pair<int, bool> temp_pair;
            if (a_intfc->ReadRecordData(temp_pair) != sizeof(temp_pair)) {
                return false;
            }
            encodedStr.push_back(temp_pair);
        }
        a_str = decodeString(encodedStr);
        return true;
    }

    inline bool write_string(SKSE::SerializationInterface* a_intfc, const std::string& a_str) {
        const auto encodedStr = encodeString(a_str);
        // i first need the size to know no of iterations
        const auto size = encodedStr.size();
        if (!a_intfc->WriteRecordData(size)) {
            return false;
        }
        for (const auto& temp_pair : encodedStr) {
            if (!a_intfc->WriteRecordData(temp_pair)) {
                return false;
            }
        }
        return true;
    };
}

namespace Serialization::V2 {
    enum class Type { Number, String, FormID };

    class Registry {
    public:
        template <class T, class Allocator>
        bool Register(std::uint32_t tag, std::uint32_t version, std::vector<T, Allocator>& data, Type type);

        template <class Map>
            requires requires { typename Map::mapped_type; }
        bool Register(std::uint32_t tag, std::uint32_t version, Map& data, Type keyType, Type valueType);

        bool Save(const SKSE::SerializationInterface* skse) const;
        bool Load(const SKSE::SerializationInterface* skse) const;

    private:
        enum class ReadResult { Ok, MissingForm, Invalid };

        struct Record {
            std::uint32_t version;
            std::function<void()> clear;
            std::function<bool(const SKSE::SerializationInterface*)> save;
            std::function<bool(const SKSE::SerializationInterface*, std::uint32_t)> load;
        };

        template <class T>
        static constexpr bool Supports(Type type);
        template <class T>
        static constexpr std::uint32_t FieldSize();
        template <class T>
        static bool Write(const SKSE::SerializationInterface* skse, const T& value, std::uint32_t& remaining);
        template <class T>
        static ReadResult Read(const SKSE::SerializationInterface* skse, T& value, Type type,
                               std::uint32_t& remaining);
        template <class Container, class... Types>
        bool RegisterContainer(std::uint32_t tag, std::uint32_t version, Container& data, Types... types);
        template <class Container>
        static bool SaveContainer(const SKSE::SerializationInterface* skse, const Container& data);
        template <class Container, class... Types>
        static bool LoadContainer(const SKSE::SerializationInterface* skse, Container& data,
                                  std::uint32_t remaining, Types... types);

        std::map<std::uint32_t, Record> records;
    };
}

template <class T, class Allocator>
bool Serialization::V2::Registry::Register(std::uint32_t tag, std::uint32_t version,
                                          std::vector<T, Allocator>& data, Type type) {
    return Supports<T>(type) && RegisterContainer(tag, version, data, type);
}

template <class Map>
    requires requires { typename Map::mapped_type; }
bool Serialization::V2::Registry::Register(std::uint32_t tag, std::uint32_t version, Map& data,
                                          Type keyType, Type valueType) {
    return Supports<typename Map::key_type>(keyType) && Supports<typename Map::mapped_type>(valueType) &&
           RegisterContainer(tag, version, data, keyType, valueType);
}

template <class T>
constexpr bool Serialization::V2::Registry::Supports(Type type) {
    switch (type) {
    case Type::Number:
        return std::is_arithmetic_v<T>;
    case Type::String:
        return std::is_same_v<T, std::string>;
    case Type::FormID:
        return std::is_same_v<T, RE::FormID>;
    }
    return false;
}

template <class T>
constexpr std::uint32_t Serialization::V2::Registry::FieldSize() {
    if constexpr (std::is_same_v<T, std::string>) {
        return sizeof(std::uint32_t);
    } else if constexpr (std::is_same_v<T, bool>) {
        return sizeof(std::uint8_t);
    } else {
        return sizeof(T);
    }
}

template <class T>
bool Serialization::V2::Registry::Write(const SKSE::SerializationInterface* skse, const T& value,
                                       std::uint32_t& remaining) {
    constexpr auto size = FieldSize<T>();
    if (size > remaining) {
        return false;
    }
    remaining -= size;
    if constexpr (std::is_same_v<T, std::string>) {
        if (value.size() > remaining) {
            return false;
        }
        const auto length = static_cast<std::uint32_t>(value.size());
        remaining -= length;
        return skse->WriteRecordData(length) && (!length || skse->WriteRecordData(value.data(), length));
    } else if constexpr (std::is_same_v<T, bool>) {
        return skse->WriteRecordData(static_cast<std::uint8_t>(value));
    } else {
        return skse->WriteRecordData(value);
    }
}

template <class T>
auto Serialization::V2::Registry::Read(const SKSE::SerializationInterface* skse, T& value, Type type,
                                      std::uint32_t& remaining) -> ReadResult {
    constexpr auto size = FieldSize<T>();
    if (size > remaining) {
        return ReadResult::Invalid;
    }
    if constexpr (std::is_same_v<T, std::string>) {
        std::uint32_t length;
        if (skse->ReadRecordDataEx(remaining, length) != size || length > remaining) {
            return ReadResult::Invalid;
        }
        value.resize(length);
        if (length && skse->ReadRecordDataEx(remaining, value.data(), length) != length) {
            return ReadResult::Invalid;
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        std::uint8_t byte;
        if (skse->ReadRecordDataEx(remaining, byte) != size || byte > 1) {
            return ReadResult::Invalid;
        }
        value = byte != 0;
    } else {
        if (skse->ReadRecordDataEx(remaining, value) != size) {
            return ReadResult::Invalid;
        }
        if constexpr (std::is_same_v<T, RE::FormID>) {
            if (type == Type::FormID && !skse->ResolveFormID(value, value)) {
                return ReadResult::MissingForm;
            }
        }
    }
    return ReadResult::Ok;
}

template <class Container, class... Types>
bool Serialization::V2::Registry::RegisterContainer(std::uint32_t tag, std::uint32_t version,
                                                   Container& data, Types... types) {
    return records.try_emplace(tag, Record{
        version,
        [&data] { data.clear(); },
        [&data](const SKSE::SerializationInterface* skse) { return SaveContainer(skse, data); },
        [&data, types...](const SKSE::SerializationInterface* skse, std::uint32_t length) {
            return LoadContainer(skse, data, length, types...);
        }
    }).second;
}

template <class Container>
bool Serialization::V2::Registry::SaveContainer(const SKSE::SerializationInterface* skse, const Container& data) {
    auto remaining = std::numeric_limits<std::uint32_t>::max();
    if (data.size() > remaining || !Write(skse, static_cast<std::uint32_t>(data.size()), remaining)) {
        return false;
    }
    for (const auto& entry : data) {
        if constexpr (requires { typename Container::mapped_type; }) {
            if (!Write(skse, entry.first, remaining) || !Write(skse, entry.second, remaining)) {
                return false;
            }
        } else {
            if (!Write<typename Container::value_type>(skse, entry, remaining)) {
                return false;
            }
        }
    }
    return true;
}

template <class Container, class... Types>
bool Serialization::V2::Registry::LoadContainer(const SKSE::SerializationInterface* skse, Container& data,
                                               std::uint32_t remaining, Types... types) {
    std::uint32_t count;
    if (Read(skse, count, Type::Number, remaining) != ReadResult::Ok) {
        return false;
    }

    constexpr bool isMap = requires { typename Container::mapped_type; };
    constexpr auto minEntrySize = [] {
        if constexpr (isMap) {
            return FieldSize<typename Container::key_type>() + FieldSize<typename Container::mapped_type>();
        } else {
            return FieldSize<typename Container::value_type>();
        }
    }();
    if (count > remaining / minEntrySize) {
        return false;
    }

    const std::array kinds{types...};
    for (std::uint32_t i = 0; i < count; ++i) {
        if constexpr (isMap) {
            typename Container::key_type key;
            typename Container::mapped_type value;
            const auto keyResult = Read(skse, key, kinds[0], remaining);
            if (keyResult == ReadResult::Invalid) {
                return false;
            }
            const auto valueResult = Read(skse, value, kinds[1], remaining);
            if (valueResult == ReadResult::Invalid) {
                return false;
            }
            if (keyResult == ReadResult::Ok && valueResult == ReadResult::Ok) {
                data.insert_or_assign(std::move(key), std::move(value));
            }
        } else {
            typename Container::value_type value;
            switch (Read(skse, value, kinds[0], remaining)) {
            case ReadResult::Ok:
                data.push_back(std::move(value));
                break;
            case ReadResult::MissingForm:
                break;
            case ReadResult::Invalid:
                return false;
            }
        }
    }
    return remaining == 0;
}

inline bool Serialization::V2::Registry::Save(const SKSE::SerializationInterface* skse) const {
    bool success = true;
    for (const auto& [tag, record] : records) {
        if (!skse->OpenRecord(tag, record.version) || !record.save(skse)) {
            success = false;
        }
    }
    return success;
}

inline bool Serialization::V2::Registry::Load(const SKSE::SerializationInterface* skse) const {
    for (const auto& [tag, record] : records) {
        record.clear();
    }
    bool success = true;
    std::uint32_t tag, version, length;
    while (skse->GetNextRecordInfo(tag, version, length)) {
        const auto entry = records.find(tag);
        if (entry == records.end() || entry->second.version != version) {
            continue;
        }
        const auto& record = entry->second;
        record.clear();
        if (!record.load(skse, length)) {
            record.clear();
            success = false;
        }
    }
    return success;
}
