#pragma once
//  ---------------------------------------------
//  Returns a human readable timestamp
//  ---------------------------------------------
//  #include "timestamp.hpp" // MG::get_human_readable_timestamp()
//  ---------------------------------------------
#include <string>
#include <chrono>
#include <format>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] std::string get_human_readable_timestamp(const std::chrono::system_clock::time_point& tp)
{
    //return std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(tp)));

    const auto days_since_epoch = std::chrono::duration_cast<std::chrono::days>(tp.time_since_epoch());
    const auto time_since_midnight = tp - std::chrono::system_clock::time_point(days_since_epoch);
    const auto date = std::chrono::year_month_day{std::chrono::sys_days{days_since_epoch}};
    const auto time = std::chrono::duration_cast<std::chrono::seconds>(time_since_midnight);

    const int year = static_cast<int>(date.year());
    const unsigned month = static_cast<unsigned>(date.month());
    const unsigned day = static_cast<unsigned>(date.day());
    const auto hour = time.count() / 3600;
    const auto min = (time.count() % 3600) / 60;
    const auto sec = time.count() % 60;

    return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", year, month, day, hour, min, sec);
}

//---------------------------------------------------------------------------
[[nodiscard]] std::string get_human_readable_timestamp()
{
    return get_human_readable_timestamp( std::chrono::system_clock::now() );
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"timestamp"> timestamp_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("MG::get_human_readable_timestamp()") = []
   {
    const auto tp = std::chrono::sys_days{std::chrono::year{2024}/std::chrono::February/1} +
                    std::chrono::hours{10} + std::chrono::minutes{3} + std::chrono::seconds{20};

    ut::expect( ut::that % MG::get_human_readable_timestamp(tp)=="2024-02-01 10:03:20"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////