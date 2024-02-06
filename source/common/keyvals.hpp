#pragma once
//  ---------------------------------------------
//  A map of string pairs
//  ---------------------------------------------
//  #include "keyvals.hpp" // MG::keyvals
//  ---------------------------------------------
#include <stdexcept> // std::runtime_error
#include "string_map.hpp" // MG::string_map<>
#include "ascii_simple_lexer.hpp" // ascii::simple_lexer


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class keyvals final
{
 public:
    using container_type = MG::string_map<std::string, std::optional<std::string>>;

 private:
    container_type m_map;

 public:
    //-----------------------------------------------------------------------
    // Get from strings like: "key1:val1,key2,key3:val3"
    void assign(const std::string_view input)
       {
        class lexer_t final : public ascii::simple_lexer<char>
        {
         private:
            std::size_t i_key_start = 0;
            std::size_t i_key_end = 0;
            std::size_t i_val_start = 0;
            std::size_t i_val_end = 0;

         public:
            explicit lexer_t(const std::string_view buf) noexcept
              : ascii::simple_lexer<char>(buf)
               {}

            [[nodiscard]] bool got_key()
               {
                while( got_data() )
                   {
                    skip_any_space();
                    if( eat(',') )
                       {
                        skip_any_space();
                       }
                    if( got(ascii::is_any_of<':','='>) )
                       {
                        throw std::runtime_error{"invalid key-value pairs"};
                       }
                    else if( got_key_token() )
                       {
                        return true;
                       }
                    else
                       {
                        get_next();
                       }
                   }
                return false;
               }

            [[nodiscard]] bool got_key_token() noexcept
               {
                i_key_start = pos();
                while( not got(ascii::is_space_or_any_of<',',':','='>) and get_next() ) ;
                i_key_end = pos();
                if( i_key_end>i_key_start )
                   {
                    skip_any_space();
                    i_val_start = i_val_end = 0u;
                    if( eat(':') or eat('=') )
                       {
                        skip_any_space();
                        i_val_start = pos();
                        while( not got(ascii::is_space_or_any_of<',',':','='>) and get_next() ) ;
                        i_val_end = pos();
                       }
                    return true;
                   }
                return false;
               }

            [[nodiscard]] std::string_view key() const noexcept { return input.substr(i_key_start, i_key_end-i_key_start); }
            [[nodiscard]] std::string_view val() const noexcept { return input.substr(i_val_start, i_val_end-i_val_start); }
        } lexer{input};


        while( lexer.got_key() )
           {
            std::optional<std::string> val;
            if( !lexer.val().empty() ) val = std::string{lexer.val()};
            m_map.insert_or_assign( std::string(lexer.key()), std::move(val));
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const noexcept { return m_map.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return m_map.is_empty(); }
    [[nodiscard]] bool contains(const std::string_view key) const noexcept { return m_map.contains(key); }

    //-----------------------------------------------------------------------
    [[nodiscard]] auto value_of(const std::string_view key) const noexcept
       {
        container_type::value_type optval;
        if( const auto it=m_map.find(key); it!=m_map.end() )
           {
            optval = it->second;
           }
        return optval;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view value_or(const std::string_view key, const std::string_view def) const noexcept
       {
        if( const auto it=m_map.find(key); it!=m_map.end() )
           {
            if( it->second.has_value() )
               {
                return it->second.value();
               }
           }
        return def;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] container_type::const_iterator begin() const noexcept { return m_map.begin(); }
    [[nodiscard]] container_type::const_iterator end() const noexcept { return m_map.end(); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
[[nodiscard]] std::string to_string(const MG::keyvals& kv)
   {
    std::string s;
    auto it = kv.begin();
    if( it!=kv.end() )
       {
        s += it->first;
        if( it->second.has_value() )
           {
            s += ':';
            s += it->second.value();
           }
        while( ++it!=kv.end() )
           {
            s += ',';
            s += it->first;
            if( it->second.has_value() )
               {
                s += ':';
                s += it->second.value();
               }
           }
       }
    return s;
   }

/////////////////////////////////////////////////////////////////////////////
static ut::suite<"MG::keyvals"> keyvals_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic") = []
   {
    MG::keyvals options;
    const std::string_view content = "key1:val1,key2,key3,key4:val4"sv;
    options.assign( content );
    ut::expect( ut::that % to_string(options)==content );

    ut::expect( ut::that % not options.is_empty() );
    ut::expect( ut::that % options.contains("key1") );
    ut::expect( ut::that % options.contains("key2") );
    ut::expect( ut::that % options.contains("key3") );
    ut::expect( ut::that % options.contains("key4") );
    ut::expect( ut::that % not options.contains("key5") );

    ut::expect( ut::that % options.value_of("key1").has_value() and options.value_of("key1").value()=="val1"sv );
    ut::expect( ut::that % not options.value_of("key2").has_value() );
    ut::expect( ut::that % not options.value_of("key3").has_value() );
    ut::expect( ut::that % options.value_of("key4").has_value() and options.value_of("key4").value()=="val4"sv );

    ut::expect( ut::that % options.value_or("key1","def"sv) == "val1"sv );
    ut::expect( ut::that % options.value_or("key2","def"sv) == "def"sv );
   };

ut::test("spaces") = []
   {
    MG::keyvals options;
    const std::string_view content = "  key1  :  val1  ,  key2  ,  key3  ,  key4  :  val4  "sv;
    const std::string_view content_trimmed = "key1:val1,key2,key3,key4:val4"sv;
    options.assign( content );
    ut::expect( ut::that % to_string(options)==content_trimmed );

    ut::expect( ut::that % not options.is_empty() );
    ut::expect( ut::that % options.contains("key1") );
    ut::expect( ut::that % options.contains("key2") );
    ut::expect( ut::that % options.contains("key3") );
    ut::expect( ut::that % options.contains("key4") );
    ut::expect( ut::that % not options.contains("key5") );

    ut::expect( ut::that % options.value_of("key1").has_value() and options.value_of("key1").value()=="val1"sv );
    ut::expect( ut::that % not options.value_of("key2").has_value() );
    ut::expect( ut::that % not options.value_of("key3").has_value() );
    ut::expect( ut::that % options.value_of("key4").has_value() and options.value_of("key4").value()=="val4"sv );
   };

// Parameterized test for accepted strings
for( auto content : std::vector<std::string_view>{""sv, "key:"sv, "key:val"sv} )
   {
    ut::test( fmt::format("ok \'{}\'", content) ) = [content]() mutable
       {
        MG::keyvals options;
        options.assign( content );
        if( content.ends_with(':') ) content.remove_suffix(1);
        ut::expect( ut::that % to_string(options)==content );
       };
   }

// Parameterized test for bad strings
for( const auto content : std::vector{"key1:val1:key2"sv, ":key"sv, ":"sv, ",:,:"sv} )
   {
    ut::test( fmt::format("bad \'{}\'", content) ) = [content]
       {
        MG::keyvals options;
        ut::expect( ut::throws([&]{ options.assign( content ); }) ) << '\"' << to_string(options) << "\" should throw\n";
       };
    }

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
