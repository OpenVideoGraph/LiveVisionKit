//     *************************** LiveVisionKit ****************************
//     Copyright (C) 2022  Sebastian Di Marco (crowsinc.dev@gmail.com)
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <https://www.gnu.org/licenses/>.
//     **********************************************************************

#include <sstream>
#include "OptionParser.hpp"

namespace clt
{

//---------------------------------------------------------------------------------------------------------------------

    bool OptionsParser::try_parse(std::deque<std::string>& args) const
    {
        if(args.empty())
            return false;

        const auto option = args.front();

        // NOTE: For duplicate alias between variabel and switch type options, we
        // prioritise parsing as a variable first, then as a switch if it fails.

        if(has_variable(option) && args.size() >= 2)
        {
            const auto argument = args[1];

            // Only 'consume' the arguments if successful
            if(m_VariableOptions.at(option)(argument))
            {
                args.pop_front();
                args.pop_front();
                return true;
            }
        }

        if(has_switch(option))
        {
            m_SwitchOptions.at(option)();
            args.pop_front();
            return true;
        }

        return false;
    }

//---------------------------------------------------------------------------------------------------------------------

    template<typename T>
    std::optional<T> OptionsParser::parse_as(const std::string& argument)
    {
        std::stringstream parser(argument);

        T value;
        parser >> value;

        if(parser.fail())
            return std::nullopt;
        else
            return value;
    }

//---------------------------------------------------------------------------------------------------------------------

    template<typename T>
    void OptionsParser::add_variable(
        const std::initializer_list<std::string>& aliases,
        const std::string& description,
        const std::function<void(T)>& callback
    )
    {
        LVK_ASSERT(!empty(aliases));

        generate_manual_entry(aliases, description, true);

        for(const auto& name : aliases)
        {
            m_VariableOptions[name] = [=](const std::string& argument)
            {
                std::optional<T> parsed_argument = parse_as<T>(argument);

                if(parsed_argument.has_value())
                {
                    callback(parsed_argument.value());
                    return true;
                }
                else
                {
                    m_ErrorHandler(name, argument);
                    return false;
                }
            };
        }
    }

//---------------------------------------------------------------------------------------------------------------------

    template<typename T>
    void OptionsParser::add_variable(
        const std::string& name,
        const std::string& description,
        const std::function<void(T)>& callback
    )
    {
        add_variable({name}, description, callback);
    }

//---------------------------------------------------------------------------------------------------------------------

    template<typename T>
    void OptionsParser::add_variable(
        const std::initializer_list<std::string>& aliases,
        const std::string& description,
        T* location
    )
    {
        LVK_ASSERT(!empty(aliases));

        generate_manual_entry(aliases, description, true);

        for(const auto& name : aliases)
        {
            m_VariableOptions[name] = [=](const std::string& argument)
            {
                std::optional<T> parsed_argument = parse_as<T>(argument);

                if(parsed_argument.has_value())
                {
                    *location = parsed_argument.value();
                    return true;
                }
                else
                {
                    m_ErrorHandler(name, argument);
                    return false;
                }
            };
        }
    }

//---------------------------------------------------------------------------------------------------------------------

    template<typename T>
    void OptionsParser::add_variable(
        const std::string& name,
        const std::string& description,
        T* location
    )
    {
        add_variable({name}, description, location);
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::add_switch(
        const std::initializer_list<std::string>& aliases,
        const std::string& description,
        bool* location
    )
    {
        LVK_ASSERT(!empty(aliases));

        generate_manual_entry(aliases, description, false);

        for(const auto& name : aliases)
        {
            m_SwitchOptions[name] = [=]()
            {
                *location = true;
            };
        }
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::add_switch(
        const std::string& name,
        const std::string& description,
        bool* location
    )
    {
        add_switch({name}, description, location);
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::add_switch(
        const std::initializer_list<std::string>& aliases,
        const std::string& description,
        const std::function<void()>& callback
    )
    {
        LVK_ASSERT(!empty(aliases));

        generate_manual_entry(aliases, description, false);

        for(const auto& name : aliases)
            m_SwitchOptions[name] = callback;
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::add_switch(
        const std::string& name,
        const std::string& description,
        const std::function<void()>& callback
    )
    {
        add_switch({name}, description, callback);
    }

//---------------------------------------------------------------------------------------------------------------------

    bool OptionsParser::has_variable(const std::string& name) const
    {
        return m_VariableOptions.contains(name);
    }

//---------------------------------------------------------------------------------------------------------------------

    bool OptionsParser::has_switch(const std::string& name) const
    {
        return m_SwitchOptions.contains(name);
    }

//---------------------------------------------------------------------------------------------------------------------

    bool OptionsParser::is_empty() const
    {
        return m_SwitchOptions.empty() && m_VariableOptions.empty();
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::set_error_handler(const OptionsParser::ErrorHandler& handler)
    {
        LVK_ASSERT(handler);

        m_ErrorHandler = handler;
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::generate_manual_entry(
        const std::initializer_list<std::string>& alias,
        const std::string& description,
        const bool has_arg
    )
    {
        LVK_ASSERT(!empty(alias));

        // Create name string
        std::string name_entry;
        for(const auto& name : alias)
        {
            if(name_entry.empty())
                name_entry += name;
            else
                name_entry += ", " + name;
        }
        if(has_arg) name_entry += " <arg>";

        m_LongestNameEntryLength = std::max(m_LongestNameEntryLength, name_entry.length());

        // Register the manual entry
        const size_t index = m_ManualEntries.size();
        m_ManualEntries.emplace_back(name_entry, description);
        for(const auto& name : alias)
            m_ManualLookup[name] = index;

        compile_manual();
    }

//---------------------------------------------------------------------------------------------------------------------

    void OptionsParser::compile_manual()
    {
        m_Manual.clear();
        for(const auto& [name_entry, description] : m_ManualEntries)
        {
            m_Manual += '\t';
            m_Manual += name_entry;
            m_Manual += std::string((m_LongestNameEntryLength - name_entry.length()) + 4, ' ');
            m_Manual += description;
            m_Manual += '\n';
        }
    }

//---------------------------------------------------------------------------------------------------------------------

    const std::string& OptionsParser::manual() const
    {
        return m_Manual;
    }

//---------------------------------------------------------------------------------------------------------------------

    std::string OptionsParser::manual(const std::string& option) const
    {
        LVK_ASSERT(m_ManualLookup.contains(option));

        const auto& [name_entry, description] = m_ManualEntries[m_ManualLookup.at(option)];
        return name_entry + '\t' + description;
    }

//---------------------------------------------------------------------------------------------------------------------

}
