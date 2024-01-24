#pragma once
//  ---------------------------------------------
//  A map of string pairs
//  ---------------------------------------------
#include <cctype> // std::isspace, ...
#include <string>
#include <string_view>
#include "string_map.hpp" // MG::string_map<>

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class keyvals final
{
 public:
    using std::string = std::string;
    using container_type = MG::string_map<std::string,std::string>;

 private:
    container_type m_map;

 public:
    //-----------------------------------------------------------------------
    // Get from strings like: "key1:val1, key2, key3:val3"
    void assign(const std::string_view s, const char sep =',') noexcept
       {
        std::size_t i = 0;     // Current character index
        while( i<s.size() )
           {
            // Skip possible spaces
            while( i<s.size() && std::isspace(s[i]) ) ++i;
            // Get key
            const std::size_t i_k0 = i;
            while( i<s.size() && !std::isspace(s[i]) && s[i]!=sep && s[i]!=':' && s[i]!='=' ) ++i;
            const std::size_t i_klen = i-i_k0;
            // Skip possible spaces
            while( i<s.size() && std::isspace(s[i]) ) ++i;
            // Get possible value
            std::size_t i_v0=0, i_vlen=0;
            if( s[i]==':' || s[i]=='=' )
               {
                // Skip key/value separator
                ++i;
                // Skip possible spaces
                while( i<s.size() && std::isspace(s[i]) ) ++i;
                // Collect value
                i_v0 = i;
                while( i<s.size() && !std::isspace(s[i]) && s[i]!=sep ) ++i;
                i_vlen = i-i_v0;
                // Skip possible spaces
                while( i<s.size() && std::isspace(s[i]) ) ++i;
               }
            // Skip possible delimiter
            if( i<s.size() && s[i]==sep ) ++i;

            // Add key if found
            if( i_klen>0 )
               {
                // Insert or assign
                m_map[std::string(s.data()+i_k0, i_klen)] = i_vlen>0 ? std::string(s.data()+i_v0, i_vlen) : ""s;
               }
           }
       }

    //-----------------------------------------------------------------------
    bool is_empty() const noexcept
       {
        return m_map.is_empty();
       }

    //-----------------------------------------------------------------------
    bool contains(const std::string_view key) const noexcept
       {
        return m_map.contains(key);
       }

    //-----------------------------------------------------------------------
    std::optional<std::string_view> value_of(const std::string_view key) const noexcept
       {
        return m_map.value_of(key);
       }
       
    //-----------------------------------------------------------------------
    //std::string string(const std::string& sep =",") const
    //   {
    //    std::string s;
    //    container_type::const_iterator i = m_map.begin();
    //    if( i!=m_map.end() )
    //       {
    //        s += i->first;
    //        if( !i->second.empty() )
    //           {
    //            s += ':';
    //            s += i->second;
    //           }
    //        while( ++i!=m_map.end() )
    //           {
    //            s += sep;
    //            s += i->first;
    //            if( !i->second.empty() )
    //               {
    //                s += ':';
    //                s += i->second;
    //               }
    //           }
    //       }
    //    return s;
    //   }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"MG::keyvals"> keyvals_tests = []
{////////////////////////////////////////////////////////////////////////////

//    ut::test("MG::keyvals") = []
//       {
//        str::keyvals options;
//        options.assign("key1:val1,key2,key3,key4:val4"sv);
//        ut::expect( ut::that %  !options.is_empty() );
//        ut::expect( ut::that %  options.contains("key1") );
//        ut::expect( ut::that %  options.contains("key2") );
//        ut::expect( ut::that %  options.contains("key3") );
//        ut::expect( ut::that %  options.contains("key4") );
//        ut::expect( ut::that %  !options.contains("key5") );
//        ut::expect( ut::that %  options.value_of("key1").has_value() );
//        ut::expect( ut::that %  options.value_of("key1").value()=="val1"s );
//        ut::expect( ut::that %  !options.value_of("key2").has_value() );
//        ut::expect( ut::that %  options.string()=="key1:val1,key2,key3,key4:val4" );
//       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
