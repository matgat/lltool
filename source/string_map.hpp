#pragma once
//  ---------------------------------------------
//  A map that preserves the original insertion
//  order optimized to use strings as keys
//  ---------------------------------------------
//  #include "string_map.hpp" // MG::string_map<>
//  ---------------------------------------------
#include <concepts>
#include <stdexcept> // std::runtime_error
#include <vector>
#include <string>
#include <string_view>
#include <optional>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG
{

template<typename T>
concept a_basic_string = std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;


/////////////////////////////////////////////////////////////////////////////
template<a_basic_string TKEY, typename TVAL> class string_map final
{
 public:
    using key_type = TKEY;
    using value_type = TVAL;
    using key_view_type = decltype( std::basic_string_view{std::declval<const key_type&>()} );
    using item_type = std::pair<key_type,value_type>;
    using container_type = std::vector<item_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

 private:
    container_type m_v;

 public:
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
            throw std::runtime_error("key already present in string_map");
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
        std::optional< std::reference_wrapper<const value_type> > val;
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
            throw std::runtime_error("key not found in string_map");
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



    //#include <concepts>
    //template <typename T> concept iterator_predicate = requires(T t) { { t(const_iterator{}) } -> std::same_as<bool>; };
    constexpr void erase_if(auto condition)
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
//---------------------------------------------------------------------------
template<typename TVAL>
[[nodiscard]] constexpr std::string to_string( MG::string_map<std::string,TVAL> const& strmap )
   {
    std::string s;
    auto it = strmap.begin();
    if( it!=strmap.end() )
       {
        s += it->first;
        s += '=';
        if constexpr(std::same_as<TVAL, std::string>)
            s += it->second;
        else
            s += std::to_string(it->second);

        while( ++it!=strmap.end() )
           {
            s += ',';
            s += it->first;
            s += '=';
            if constexpr(std::same_as<TVAL, std::string>)
                s += it->second;
            else
                s += std::to_string(it->second);
           }
       }
    return s;
   }
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"MG::string_map<>"> string_map_tests = []
{////////////////////////////////////////////////////////////////////////////
    using ut::expect;
    using ut::that;
    using ut::throws;

    ut::test("basic operations") = []
       {
        MG::string_map<std::string,std::string> v;
        expect( that % v.is_empty() and v.size()==0u );

        // Inserting
        v.insert_or_assign("key1","val1");
        expect( that % v.size()==1u and to_string(v)=="key1=val1"s );

        v.insert_if_missing("key2","old2");
        expect( that % v.size()==2u and to_string(v)=="key1=val1,key2=old2"s );

        v.insert_unique("key3","val3");
        expect( that % v.size()==3u and to_string(v)=="key1=val1,key2=old2,key3=val3"s );

        v.insert_if_missing("key2","val2");
        expect( that % v.size()==3u and to_string(v)=="key1=val1,key2=old2,key3=val3"s ) << "shouldn't modify already existing key\n";

        // Overwriting
        v.insert_or_assign("key2","val2");
        expect( that % v.size()==3u and to_string(v)=="key1=val1,key2=val2,key3=val3"s ) << "should have modified key2\n";
        expect( throws<std::runtime_error>([v] { ut::mut(v).insert_unique("key1",""); })) << "should throw runtime_error inserting already existing key\n";

        // Accessing not existing
        expect( throws<std::runtime_error>([&v] { [[maybe_unused]] auto s = v["x"]; })) << "should throw runtime_error accessing not existing key\n";
        expect( !v.value_of("x").has_value() );
        expect( that % v.value_or("x","def")=="def"s );

        // Accessing existing
        expect( that % v["key1"]=="val1"s and v["key2"]=="val2"s and v["key3"]=="val3"s );
        expect( that % v.value_of("key3")->get()=="val3"s );
        expect( that % v.value_or("key3","def")=="val3"s );

        // Erasing
        v.erase("x");
        expect( that % v.size()==3u and to_string(v)=="key1=val1,key2=val2,key3=val3"s ) << "erasing not existing key does nothing\n";
        v.erase("key1");
        expect( that % v.size()==2u and to_string(v)=="key2=val2,key3=val3"s ) << "erasing first key\n";
        v.clear();
        expect(that % v.is_empty() and v.size()==0u) << "should be empty after clear\n";
       };


    ut::test("loop erase") = []
       {
        MG::string_map<std::string,int> v;
        v.insert_unique("1",1);
        v.insert_unique("2",2);
        v.insert_unique("3",3);
        v.insert_unique("4",4);
        v.insert_unique("5",5);
        expect( that % to_string(v)=="1=1,2=2,3=3,4=4,5=5"s );

        // Erase odd values
        v.erase_if( [](decltype(v)::const_iterator it) constexpr -> bool { return it->second % 2; } );
        expect( that % to_string(v)=="2=2,4=4"s );
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
