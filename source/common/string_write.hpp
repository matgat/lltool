#pragma once
//  ---------------------------------------------
//  A stream interface that writes to string
//  ---------------------------------------------
//  #include "string_write.hpp" // MG::string_write
//  ---------------------------------------------
#include <string>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class string_write final
{
 private:
    std::string m_string;

 public:
    string_write& operator<<(const char c)
       {
        m_string += c;
        return *this;
       }

    string_write& operator<<(const std::string_view sv)
       {
        m_string += sv;
        return *this;
       }

    [[nodiscard]] const std::string& str() const noexcept { return m_string; }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"MG::string_write"> string_write_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("streaming in") = []
   {
    MG::string_write ss;

    ut::expect( ut::that % ss.str()==""sv );
    
    ss << 'c' << "iao"sv;
    ut::expect( ut::that % ss.str()=="ciao"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
