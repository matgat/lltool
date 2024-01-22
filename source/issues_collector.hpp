#pragma once
//  ---------------------------------------------
//  Abstract the issues notification mechanism
//  #include "issues_collector.hpp" // MG::issues
//  ---------------------------------------------
#include <string>
#include <vector>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG
{


/////////////////////////////////////////////////////////////////////////////
class issues final
{
    std::vector<std::string> m_issues;

 public:
    [[nodiscard]] constexpr std::size_t size() const noexcept { return m_issues.size(); }

    constexpr void operator()(std::string&& txt)
       {
        m_issues.push_back( std::move(txt) );
       }

    [[nodiscard]] constexpr auto begin() const noexcept { return m_issues.cbegin(); }
    [[nodiscard]] constexpr auto end() const noexcept { return m_issues.cend(); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
