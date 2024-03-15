#pragma once
//  ---------------------------------------------
//  A map implemented with vector optimized to
//  use strings as keys (string_views in queries)
//  .Preserves the original insertion order
//  .Cache friendly, performance degrades with N
//  ---------------------------------------------
//  #include "string_map.hpp" // MG::string_map<>
//  ---------------------------------------------
#include <utility> // std::pair
#include <vector>
#include <string>
#include <string_view>
#include <concepts> // std::same_as<>, std::predicate
#include <optional>
//#include <initializer_list>
#include <stdexcept> // std::runtime_error
#include <format>



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG
{

template<typename T>
concept StdBasicString = std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;

//template<typename T>
//concept ConvertibleToStringView = std::convertible_to<T, std::string_view>;

//template<typename T>
//concept formattable = requires (T& v, std::format_context ctx) { std::formatter<std::remove_cvref_t<T>>().format(v, ctx); };

/////////////////////////////////////////////////////////////////////////////
template<StdBasicString TKEY, typename TVAL> class string_map final
{
 public:
    using key_type = TKEY;
    using value_type = TVAL;
    using opt_refvalue_type = std::optional< std::reference_wrapper<const value_type> >;
    using key_view_type = decltype( std::basic_string_view{std::declval<const key_type&>()} );
    using item_type = std::pair<key_type,value_type>;
    using container_type = std::vector<item_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

 private:
    container_type m_v;

 public:
    //constexpr string_map(std::initializer_list<item_type> lst)
    //  : m_v{lst}
    //   {}

    [[nodiscard]] constexpr auto size() const noexcept { return m_v.size(); }
    [[nodiscard]] constexpr bool is_empty() const noexcept { return m_v.empty(); }

    [[nodiscard]] constexpr bool operator==(string_map<TKEY,TVAL> const& other) const noexcept
       {
        if( size()!=other.size() )
           {
            return false;
           }
        for( const_iterator it_other=other.begin(); it_other!=other.end(); ++it_other )
           {
            const_iterator it_mine = find(it_other->first);
            if( it_mine==end() or it_mine->second!=it_other->second )
               {
                return false;
               }
           }
        return true;
       }


    [[maybe_unused]] constexpr value_type& insert_if_missing(key_type&& key, value_type&& val)
       {
        if( iterator it=find(key); it!=end() )
           {
            return it->second; // Already present, do nothing
           }
        return append( item_type{std::move(key), std::move(val)} );
       }

    [[maybe_unused]] constexpr value_type& insert_or_assign(key_type&& key, value_type&& val)
       {
        if( iterator it=find(key); it!=end() )
           {
            return it->second = val; // Already present, overwrite
           }
        return append( item_type{std::move(key), std::move(val)} );
       }

    [[maybe_unused]] constexpr value_type& insert_unique(key_type&& key, value_type&& val)
       {
        if( contains(key) )
           {
            if constexpr(std::formattable<key_type,char>)
                 throw std::runtime_error{ std::format("key '{}' already present in stringmap", key) };
            else throw std::runtime_error{ "key already present in stringmap" };
           }
        return append( item_type{std::move(key), std::move(val)} );
       }

    [[maybe_unused]] constexpr value_type& append(item_type&& item)
       {
        m_v.push_back( std::move(item) );
        return m_v.back().second;
       }



    [[nodiscard]] constexpr const_iterator find(const key_view_type key) const noexcept
       {
        const_iterator it = m_v.begin();
        while( it!=m_v.end() )
           {
            if( key==it->first ) break;
            ++it;
           }
        return it;
       }

    [[nodiscard]] constexpr iterator find(const key_view_type key) noexcept
       {
        iterator it = m_v.begin();
        while( it!=m_v.end() )
           {
            if( key==it->first ) break;
            ++it;
           }
        return it;
       }

    // Deducing this:
    //template<class Self> [[nodiscard]] constexpr auto&& xxx(this Self&& self)
    //   {
    //    std::forward<Self>(self)
    //   }

    [[nodiscard]] constexpr bool contains(const key_view_type key) const noexcept
       {
        return find(key)!=end();
       }

    [[nodiscard]] constexpr auto value_of(const key_view_type key) const noexcept
       {
        opt_refvalue_type val;
        if( const_iterator it=find(key); it!=end() )
           {
            val = std::cref(it->second);
           }
        return val;
       }

    [[nodiscard]] constexpr value_type const& value_or(const key_view_type key, value_type const& def) const noexcept
       {
        if( const_iterator it=find(key); it!=end() )
           {
            return it->second;
           }
        return def;
       }

    [[nodiscard]] constexpr value_type const& operator[](const key_view_type key) const
       {
        const_iterator it = find(key);
        if( it==end() )
           {
            if constexpr(std::formattable<key_view_type,char>)
                 throw std::runtime_error{ std::format("key '{}' not found in string_map", key) };
            else throw std::runtime_error{ "key not found in string_map" };
           }
        return it->second;
       }

    [[nodiscard]] constexpr value_type extract_or(const key_view_type key, value_type const& def) noexcept
       {
        value_type val{def};
        if( const_iterator it=find(key); it!=end() )
           {
            val = it->second;
            it = erase(it);
           }
        return val;
       }

    constexpr void erase_if(std::predicate<const_iterator> auto condition)
       {
        for( const_iterator it=begin(); it!=end(); )
           {
            if( condition(it) )
               {
                it = erase(it);
               }
            else
               {
                ++it;
               }
           }
       }

    constexpr void erase(const key_view_type key)
       {
        for( const_iterator it = m_v.begin(); it!=m_v.end(); ++it )
           {
            if( key==it->first )
               {
                it = m_v.erase(it);
                break;
               }
           }
       }

    [[nodiscard]] constexpr const_iterator erase(const_iterator it)
       {
        return m_v.erase(it);
       }

    constexpr void clear() noexcept
       {
        m_v.clear();
       }


    [[nodiscard]] constexpr const_iterator begin() const noexcept { return m_v.begin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return m_v.end(); }
    [[nodiscard]] constexpr iterator begin() noexcept { return m_v.begin(); }
    [[nodiscard]] constexpr iterator end() noexcept { return m_v.end(); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
template<typename TVAL>
[[nodiscard]] constexpr std::string to_string(MG::string_map<std::string,TVAL> const& strmap)
   {
    std::string s;
    auto it = strmap.begin();
    if( it!=strmap.end() )
       {
        s = std::format("{}={}", it->first, it->second);
        while( ++it!=strmap.end() )
           {
            s += std::format(",{}={}", it->first, it->second);
           }
       }
    return s;
   }
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"MG::string_map<>"> string_map_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic operations") = []
   {
    MG::string_map<std::string,std::string> m;
    ut::expect( m.is_empty() );
    ut::expect( ut::that % m.size()==0u );

    // Inserting
    m.insert_or_assign("key1","val1");
    ut::expect( ut::that % m.size()==1u );
    ut::expect( ut::that % to_string(m)=="key1=val1"sv );

    m.insert_if_missing("key2","old2");
    ut::expect( ut::that % m.size()==2u );
    ut::expect( ut::that % to_string(m)=="key1=val1,key2=old2"sv );

    m.insert_unique("key3","val3");
    ut::expect( ut::that % m.size()==3u );
    ut::expect( ut::that % to_string(m)=="key1=val1,key2=old2,key3=val3"sv );

    m.insert_if_missing("key2","val2");
    ut::expect( ut::that % m.size()==3u );
    ut::expect( ut::that % to_string(m)=="key1=val1,key2=old2,key3=val3"sv ) << "shouldn't modify already existing key\n";

    // Overwriting
    m.insert_or_assign("key2","val2");
    ut::expect( ut::that % m.size()==3u );
    ut::expect( ut::that % to_string(m)=="key1=val1,key2=val2,key3=val3"sv ) << "should have modified key2\n";
    ut::expect( ut::throws<std::runtime_error>([m] { ut::mut(m).insert_unique("key1",""); })) << "should throw already existing key\n";

    // Accessing not existing
    ut::expect( ut::throws<std::runtime_error>([&m] { [[maybe_unused]] auto s = m["x"]; })) << "should throw not existing key\n";
    ut::expect( not m.value_of("x").has_value() );
    ut::expect( ut::that % m.value_or("x","def")=="def"sv );

    // Accessing existing
    ut::expect( ut::that % m["key1"]=="val1"sv );
    ut::expect( ut::that % m["key2"]=="val2"sv );
    ut::expect( ut::that % m["key3"]=="val3"sv );
    ut::expect( ut::that % m.value_of("key3")->get()=="val3"sv );
    ut::expect( ut::that % m.value_or("key3","def")=="val3"sv );

    // Erasing
    m.erase("x");
    ut::expect( ut::that % m.size()==3u );
    ut::expect( ut::that % to_string(m)=="key1=val1,key2=val2,key3=val3"sv ) << "erasing not existing key does nothing\n";
    m.erase("key1");
    ut::expect( ut::that % m.size()==2u );
    ut::expect( ut::that % to_string(m)=="key2=val2,key3=val3"sv ) << "key1 should be erased\n";
    m.clear();
    ut::expect( m.is_empty() ) << "should be empty after clear\n";
    ut::expect( ut::that % m.size()==0u );
   };


ut::test("loop erase") = []
   {
    MG::string_map<std::string,int> m;
    m.insert_unique("1",1);
    m.insert_unique("2",2);
    m.insert_unique("3",3);
    m.insert_unique("4",4);
    m.insert_unique("5",5);
    ut::expect( ut::that % to_string(m)=="1=1,2=2,3=3,4=4,5=5"sv );

    // Erase odd values
    m.erase_if( [](decltype(m)::const_iterator it) noexcept -> bool { return it->second % 2; } );
    ut::expect( ut::that % to_string(m)=="2=2,4=4"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
