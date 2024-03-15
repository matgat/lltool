#pragma once
//  ---------------------------------------------
//  Abstract the issues notification mechanism
//  ---------------------------------------------
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
    [[nodiscard]] constexpr std::string const& at(const std::size_t idx) const { return m_issues.at(idx); }
    constexpr void operator()(std::string&& txt) { m_issues.push_back( std::move(txt) ); }

    [[nodiscard]] constexpr auto begin() const noexcept { return m_issues.cbegin(); }
    [[nodiscard]] constexpr auto end() const noexcept { return m_issues.cend(); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <format>
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"issues_collector"> issues_collector_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic usage") = []
   {
    MG::issues notify_issue;
    notify_issue("issue1");
    notify_issue("issue2");
    notify_issue("issue3");

    ut::expect( ut::that %  notify_issue.size() == 3u );

    std::size_t i = 0u;
    for( const auto& issue : notify_issue )
       {
        ut::expect( ut::that %  issue == std::format("issue{}",++i) );
       }
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
