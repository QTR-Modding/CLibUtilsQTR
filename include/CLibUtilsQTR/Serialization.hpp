#pragma once
#include <map>
#include <mutex>

namespace SKSE{
    class SerializationInterface;
}

namespace Serialization {

    // Credits: https:// github.com/ozooma10/OSLAroused/blob/29ac62f220fadc63c829f6933e04be429d4f96b0/src/PersistedData.cpp
    // BaseData is based off how powerof3's did it in Afterlife
    template <typename T,typename U>
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
                          std::uint32_t) {return false;}
        virtual bool Save(SKSE::SerializationInterface*) {return false;}
        virtual bool Load(SKSE::SerializationInterface*) {return false;}

        void Clear() {
            Locker locker(m_Lock);
            m_Data.clear();
        }

    protected:
        ~BaseData() = default;
        std::map<T,U> m_Data;

        using Lock = std::recursive_mutex;
        using Locker = std::lock_guard<Lock>;
        mutable Lock m_Lock;
    };

    inline std::vector<std::pair<int, bool>> encodeString(const std::string& inputString)
    {
        std::vector<std::pair<int, bool>> encodedValues;
        try {
            for (int i = 0; i < 100 && inputString[i] != '\0'; i++) {
                const char ch = inputString[i];
                if (std::isprint(ch) && (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) && ch >= 0) {
                    encodedValues.emplace_back(static_cast<int>(ch), std::isupper(ch));
                }
            }
        } catch (const std::exception& e) {
            logger::error("Error encoding string: {}", e.what());
            return encodeString("ERROR");
        }
        return encodedValues;
    }

    inline std::string decodeString(const std::vector<std::pair<int, bool>>& encodedValues)
    {
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

    inline bool read_string(SKSE::SerializationInterface* a_intfc, std::string& a_str)
    {
        std::vector<std::pair<int, bool>> encodedStr;
	    std::size_t size;
        if (!a_intfc->ReadRecordData(size)) {
            return false;
        }
        for (std::size_t i = 0; i < size; i++) {
            std::pair<int, bool> temp_pair;
            if (!a_intfc->ReadRecordData(temp_pair)) {
			    return false;
		    }
            encodedStr.push_back(temp_pair);
	    }
        a_str = decodeString(encodedStr);
	    return true;
    }

    inline bool write_string(SKSE::SerializationInterface* a_intfc, const std::string& a_str)
    {
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
